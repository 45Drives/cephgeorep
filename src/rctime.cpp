/*
 *    Copyright (C) 2019-2021 Joshua Boudreau
 *    
 *    This file is part of cephgeorep.
 * 
 *    cephgeorep is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License, or
 *    (at your option) any later version.
 * 
 *    cephgeorep is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License
 *    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "rctime.hpp"
#include "alert.hpp"
#include "signal.hpp"
#include <fstream>
#include <sys/xattr.h>
#include <sys/stat.h>

inline bool operator>(const timespec &lhs, const timespec &rhs){
	return (lhs.tv_sec > rhs.tv_sec) ||
	((lhs.tv_sec == rhs.tv_sec) && (lhs.tv_nsec > rhs.tv_nsec));
}

LastRctime::LastRctime(const fs::path &last_rctime_path) : last_rctime_path_(last_rctime_path){
	Logging::log.message("Reading last rctime from disk.", 2);
	std::ifstream f(last_rctime_path_.string());
	std::string str;
	if(!f){
		init_last_rctime();
		f.open(last_rctime_path_.string());
	}
	try{
		getline(f, str, '.');
		last_rctime_.tv_sec = (time_t)stoul(str);
		getline(f, str);
		last_rctime_.tv_nsec = stol(str);
	}catch(const std::invalid_argument &){
		// last_rctime.dat corrupted
		Logging::log.warning(last_rctime_path_.string() + " is corrupt. Reinitializing.");
		init_last_rctime();
		last_rctime_.tv_sec = last_rctime_.tv_nsec = 0;
	}
	set_signal_handlers(this);
}

LastRctime::~LastRctime(void){
	write_last_rctime();
}

void LastRctime::write_last_rctime(void) const{
	Logging::log.message("Writing last rctime to disk.", 2);
	std::ofstream f(last_rctime_path_.string());
	if(!f){
		init_last_rctime();
		f.open(last_rctime_path_.string());
	}
	f << last_rctime_.tv_sec << '.' << last_rctime_.tv_nsec << std::endl;
	f.close();
}

void LastRctime::init_last_rctime(void) const{
	boost::system::error_code ec;
	Logging::log.message(last_rctime_path_.string() + " does not exist. Creating and initializing to 0.0.", 2);
	fs::create_directories(last_rctime_path_.parent_path(), ec);
	if(ec) Logging::log.error("Cannot create path: " + last_rctime_path_.parent_path().string());
	std::ofstream f(last_rctime_path_.string());
	f << "0.0" << std::endl;
	f.close();
}

bool LastRctime::is_newer(const fs::path &path) const{
	return get_rctime(path) > last_rctime_;
}

timespec LastRctime::get_rctime(const fs::path &path) const{
	timespec rctime;
	if(is_directory(path)){
		char value[XATTR_SIZE];
		if(getxattr(path.c_str(), "ceph.dir.rctime", value, XATTR_SIZE) == -1){
			Logging::log.warning("Cannot read ceph.dir.rctime of " + path.string() + "\nIgnoring " + path.string());
			rctime.tv_sec = 0;
			rctime.tv_nsec = 0;
		}else{
			std::string str(value);
			rctime.tv_sec = (time_t)stoul(str.substr(0,str.find('.')));
			// value = <seconds> + '.09' + <nanoseconds>
			// +3 is to remove the '.09'
			rctime.tv_nsec = stol(str.substr(str.find('.')+3, std::string::npos));
		}
	}else{ // file
		struct stat t_stat;
		if(lstat(path.c_str(), &t_stat) == -1){
			Logging::log.warning("Cannot read mtime of " + path.string() + "\nIgnoring " + path.string());
			rctime.tv_sec = 0;
			rctime.tv_nsec = 0;
		}else{
			rctime.tv_sec = t_stat.st_mtim.tv_sec;
			rctime.tv_nsec = t_stat.st_mtim.tv_nsec;
		}
	}
	return rctime;
}

bool LastRctime::check_for_change(const fs::path &path, timespec &new_rctime) const{
	bool change = false;
	timespec temp_rctime;
	for(fs::directory_iterator itr(path); itr != fs::directory_iterator(); *itr++){
		if((temp_rctime = get_rctime(*itr)) > last_rctime_){
			change = true;
			if(temp_rctime > new_rctime) new_rctime = temp_rctime; // get highest
		}
	}
	return change;
}

void LastRctime::update(const timespec &new_rctime){
	last_rctime_.tv_sec = new_rctime.tv_sec;
	last_rctime_.tv_nsec = new_rctime.tv_nsec;
}

std::string &operator+(std::string lhs, const timespec &rhs){
	return lhs.append(std::to_string(rhs.tv_sec) + "." + std::to_string(rhs.tv_nsec));
}

std::string &operator+(const timespec &lhs, std::string rhs){
	return (std::to_string(lhs.tv_sec) + "." + std::to_string(lhs.tv_nsec)).append(rhs);
}

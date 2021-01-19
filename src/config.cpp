/*
 *    Copyright (C) 2019-2021 Joshua Boudreau <jboudreau@45drives.com>
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

#include "config.hpp"
#include "alert.hpp"
#include <boost/system/error_code.hpp>
#include <fstream>
#include <sstream>

inline void strip_whitespace(std::string &str){
	std::size_t strItr;
	// back ws
	if((strItr = str.find('#')) == std::string::npos){ // strItr point to '#' or end
		strItr = str.length();
	}
	strItr--; // point to last character
	while(strItr && (str.at(strItr) == ' ' || str.at(strItr) == '\t')){ // remove whitespace
		strItr--;
	} // strItr points to last character
	str = str.substr(0,strItr + 1);
	// front ws
	strItr = 0;
	while(strItr < str.length() && (str.at(strItr) == ' ' || str.at(strItr) == '\t')){ // remove whitespace
		strItr++;
	} // strItr points to first character
	str = str.substr(strItr, str.length() - strItr);
}

Config::Config(const fs::path &config_path, const ConfigOverrides &config_overrides){
	std::string line, key, value;
	
	// open file
	std::fstream config_file(config_path.string());
	if(!config_file){
		config_file.close();
		init_config_file(config_path);
		config_file.open(config_path.string());
	}
	
	// read file
	while(config_file){
		getline(config_file, line);
		// full line comments:
		if(line.empty() || line.front() == '#')
			continue; // ignore comments
			
		std::stringstream linestream(line);
		getline(linestream, key, '=');
		getline(linestream, value);
		
		strip_whitespace(key);
		strip_whitespace(value);
		
		if(key == "Source Directory"){
			base_path_ = fs::path(value);
		}else if(key == "Remote User"){
			remote_user_ = value;
		}else if(key == "Remote Host"){
			remote_host_ = value;
		}else if(key == "Remote Directory"){
			remote_directory_ = fs::path(value);
		}else if(key == "Metadata Directory"){
			last_rctime_path_ = fs::path(value).append(LAST_RCTIME_NAME);
		}else if(key == "Sync Period"){
			try{
				sync_period_s_ = std::chrono::seconds(stoi(value));
			}catch(const std::invalid_argument &){
				sync_period_s_ = std::chrono::seconds(-1);
			}
		}else if(key == "Ignore Hidden"){
			std::istringstream(value) >> std::boolalpha >> ignore_hidden_ >> std::noboolalpha;
		}else if(key == "Ignore Windows Lock"){
			std::istringstream(value) >> std::boolalpha >> ignore_win_lock_ >> std::noboolalpha;
		}else if(key == "Ignore Vim Swap"){
			std::istringstream(value) >> std::boolalpha >> ignore_vim_swap_ >> std::noboolalpha;
		}else if(key == "Propagation Delay"){
			try{
				prop_delay_ms_ = std::chrono::milliseconds(stoi(value));
			}catch(const std::invalid_argument &){
				prop_delay_ms_ = std::chrono::milliseconds(-1);
			}
		}else if(key == "Log Level"){
			try{
				log_level_ = stoi(value);
				Logging::log = Logger(log_level_);
			}catch(const std::invalid_argument &){
				log_level_ = -1;
			}
		}else if(key == "Exec"){
			exec_bin_ = value;
		}else if(key == "Flags"){
			exec_flags_ = value;
		}else if(key == "Processes"){
			try{
				nproc_ = stoi(value);
			}catch(const std::invalid_argument &){
				nproc_ = -1;
			}
		}else if(key == "Threads"){
			try{
				threads_ = stoi(value);
			}catch(const std::invalid_argument &){
				threads_ = -1;
			}
		}
		// else ignore entry
	}
	
	override_fields(config_overrides);
	
	verify(config_path);
	
	if(log_level_ >= 2) dump();
}

void Config::override_fields(const ConfigOverrides &config_overrides){
	if(config_overrides.log_level_override.overridden()){
		log_level_ = config_overrides.log_level_override.value();
		Logging::log = Logger(log_level_);
	}
	if(config_overrides.nproc_override.overridden()){
		nproc_ = config_overrides.nproc_override.value();
	}
	if(config_overrides.threads_override.overridden()){
		threads_ = config_overrides.threads_override.value();
	}
}

void Config::verify(const fs::path &config_path) const{
	bool errors = false;
	if(base_path_.empty()){
		Logging::log.error("config does not contain a search directory (Source Directory)", false);
		errors = true;
	}
	if(remote_host_.empty()){
		Logging::log.warning("config does not contain a remote host (Remote Host)");
		// just warning
	}
	if(remote_directory_.empty()){
		Logging::log.warning("config does not contain a remote destination (Remote Directory)");
		// just warning
	}
	if(last_rctime_path_.empty()){
		Logging::log.error("config does not contain a metadata path (Metadata Directory)", false);
		errors = true;
	}
	if(sync_period_s_ < std::chrono::seconds(0)){
		Logging::log.error("sync frequency must be positive integer (Sync Period)", false);
		errors = true;
	}
	if(prop_delay_ms_ < std::chrono::milliseconds(0)){
		Logging::log.error("rctime prop delay must be positive integer (Propagation Delay)", false);
		errors = true;
	}
	if(log_level_ < 0){
		Logging::log.error("log level must be positive integer (Log Level)", false);
		errors = true;
	}
	if(exec_bin_.empty()){
		Logging::log.error("config must contain a program to execute! (Exec)", false);
		errors = true;
	}
	if(exec_flags_.empty()){
		Logging::log.warning("no execution flags present in config (Flags)");
		// just warning
	}
	if(nproc_ < 0){
		Logging::log.error("number of processses must be positive integer (Processes)", false);
		errors = true;
	}
	if(threads_ < 0){
		Logging::log.error("number of threads must be positive integer (Processes)", false);
		errors = true;
	}
	if(errors){
		Logging::log.error("Please fix these mistakes in " + config_path.string());
	}
}

void Config::init_config_file(const fs::path &config_path) const{
	boost::system::error_code ec;
	fs::create_directories(config_path.parent_path(), ec);
	if(ec) Logging::log.error("Error creating path: " + config_path.parent_path().string());
	std::ofstream f(config_path.string());
	if(!f) Logging::log.error("Error opening config file: " + config_path.string());
	f <<
	"# local backup settings\n"
	"Source Directory =               # full path to directory to backup\n"
	"Ignore Hidden = false            # ignore files beginning with \".\"\n"
	"Ignore Windows Lock = true       # ignore files beginning with \"~$\"\n"
	"Ignore Vim Swap = true           # ignore vim .swp files (.<filename>.swp)\n"
	"\n"
	"# remote settings\n"
	"Remote User =                    # user on remote backup machine (optional)\n"
	"Remote Host =                    # remote backup machine address/host\n"
	"Remote Directory =               # directory in remote backup\n"
	"\n"
	"# daemon settings\n"
	"Exec = rsync                     # program to use for syncing - rsync or scp\n"
	"Flags = -a --relative            # execution flags for above program (space delim)\n"
	"Metadata Directory = /var/lib/ceph/cephfssync/\n"
	"Sync Period = 10                 # time in seconds between checks for changes\n"
	"Propagation Delay = 100          # time in milliseconds between snapshot and sync\n"
	"Processes = 1                    # number of parallel sync processes to launch\n"
	"Threads = 8                      # number of worker threads to search for files\n"
	"Log Level = 1\n"
	"# 0 = minimum logging\n"
	"# 1 = basic logging\n"
	"# 2 = debug logging\n"
	"# If Remote User is empty, the daemon will sync remotely as the executing user.\n"
	"# Propagation Delay is to account for the limit that Ceph can\n"
	"# propagate the modification time of a file all the way back to\n"
	"# the root of the sync directory.\n";
	f.close();
}

void Config::dump(void) const{
	std::stringstream ss;
	ss << "configuration:" << std::endl;
	ss << std::endl;
	ss << "source settings:" << std::endl;
	ss << "Source Directory =" << base_path_.string() << std::endl;
	ss << "Ignore Hidden = " << std::boolalpha << ignore_hidden_ << std::endl;
	ss << "Ignore Windows Lock = " << std::boolalpha << ignore_win_lock_ << std::endl;
	ss << "Ignore Vim Swap = " << std::boolalpha << ignore_vim_swap_ << std::endl;
	ss << std::endl;
	ss << "remote settings:" << std::endl;
	ss << "Remote User = " << remote_user_ << std::endl;
	ss << "Remote Host = " << remote_host_ << std::endl;
	ss << "Remote Directory = " << remote_directory_.string() << std::endl;
	ss << std::endl;
	ss << "daemon settings:" << std::endl;
	ss << "Exec = " << exec_bin_ << std::endl;
	ss << "Flags = " << exec_flags_ << std::endl;
	ss << "Metadata Directory = " << last_rctime_path_.string() << std::endl;
	ss << "Sync Period = " << sync_period_s_.count() << " (seconds)" << std::endl;
	ss << "Propagation Delay = " << prop_delay_ms_.count() << " (milliseconds)" << std::endl;
	ss << "Processes = " << nproc_ << std::endl;
	ss << "Threads = " << threads_ << std::endl;
	ss << "Log Level = " << log_level_ << std::endl;
	Logging::log.message(ss.str(), 2);
}

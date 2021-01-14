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

#include "crawler.hpp"
#include "alert.hpp"
#include "rctime.hpp"
#include "config.hpp"
// #include "exec.hpp"
#include <boost/filesystem.hpp>
#include <sys/xattr.h>
#include <thread>
#include <chrono>

namespace fs = boost::filesystem;

Crawler::Crawler(const fs::path &config_path, size_t envp_size, const ConfigOverrides &config_overrides)
		: config_(config_path, config_overrides)
		, last_rctime_(config_.last_rctime_path_)
		, syncer(envp_size, config_){
	base_path_ = config_.base_path_;
}

void Crawler::reset(void){
	file_list_.clear();
}

void Crawler::poll_base(bool seed){
	timespec new_rctime;
	Logging::log.message("Watching: " + base_path_.string(),1);
	if(seed) last_rctime_.update({1}); // sync everything
	do{
		auto start = std::chrono::system_clock::now();
		if(last_rctime_.check_for_change(base_path_, new_rctime)){
			Logging::log.message("Change detected in " + base_path_.string(), 1);
			uintmax_t total_bytes = 0;
			// take snapshot
			fs::path snap_path = create_snap(new_rctime);
			// wait for rctime to trickle to root
			std::this_thread::sleep_for(config_.prop_delay_ms_);
			// queue files
			trigger_search(snap_path, total_bytes);
			Logging::log.message("New files to sync: "+std::to_string(file_list_.size()),1);
			// launch rsync
			if(!file_list_.empty()){
				// TODO: make Syncer class and call sync method here
				//launch_procs(sync_queue, config.rsync_nproc, total_bytes);
			}
			// clear sync queue
			reset();
			// delete snapshot
			delete_snap(snap_path);
			// overwrite last_rctime
			last_rctime_.update(new_rctime);
		}
		auto end = std::chrono::system_clock::now();
		std::chrono::seconds elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
		if(elapsed < config_.sync_period_s_ && !seed) // if it took longer than sync freq, don't wait
			std::this_thread::sleep_for(config_.sync_period_s_ - elapsed);
	}while(!seed);
}

fs::path Crawler::create_snap(const timespec &rctime) const{
	boost::system::error_code ec;
	std::string pid = std::to_string(getpid());
	fs::path snap_path = fs::path(base_path_).append(".snap/"+pid+"snapshot"+rctime);
	Logging::log.message("Creating snapshot: " + snap_path.string(), 2);
	fs::create_directories(snap_path, ec);
	if(ec) Logging::log.error("Error creating path: " + snap_path.string());
	return snap_path;
}

void Crawler::trigger_search(const fs::path &snap_path, uintmax_t &total_bytes){
	// launch crawler in snapshot
	Logging::log.message("Launching crawler",2);
	// seed recursive function with snap_path
	find_new_files_recursive(snap_path, snap_path, total_bytes);
	// log list of new files
	if(config_.log_level_ >= 2){ // skip loop if not logging
		Logging::log.message("Files to sync:",2);
		for(auto i : file_list_){
			Logging::log.message(i.string(),2);
		}
	}
}

bool Crawler::ignore_entry(const fs::directory_entry &entry) const{
	return (config_.ignore_hidden_ == true && entry.path().filename().string().front() == '.')
		|| (config_.ignore_win_lock_  == true && entry.path().filename().string().substr(0,2) == "~$")
		|| !last_rctime_.is_newer(entry);
}

void Crawler::find_new_files_recursive(fs::path current_path, const fs::path &snap_root, uintmax_t &total_bytes){
	for(fs::directory_iterator itr{current_path}; itr != fs::directory_iterator{}; *itr++){
		fs::directory_entry entry = *itr;
		if(ignore_entry(entry)) continue;
		if(is_directory(entry)){
			find_new_files_recursive(current_path/(entry.path().filename()), snap_root, total_bytes); // recurse
		}else{
			// cut path at sync dir for rsync /sync_dir/.snap/snapshotX/./rel_path/file
			total_bytes += fs::file_size(entry.path());
			file_list_.emplace_back(snap_root/fs::path(".")/fs::relative(entry.path(), snap_root));
		}
	}
}

void Crawler::delete_snap(const fs::path &path) const{
	boost::system::error_code ec;
	Logging::log.message("Removing snapshot: " + path.string(), 2);
	fs::remove(path, ec);
	if(ec) Logging::log.error("Error creating path: " + path.string());
}

// -----------------------------

// void sigint_hdlr(int signum){
// 	switch(signum){
// 		case SIGTERM:
// 		case SIGINT:
// 			// cleanup from termination
// 			writeLast_rctime(last_rctime);
// 			exit(EXIT_SUCCESS);
// 		default:
// 			exit(signum);
// 	}
// }
// 
// void initDaemon(void){
// 	loadConfig();
// 	Log("Starting Ceph Georep Daemon.",1);
// 	if(config.log_level >= 2) dumpConfig();
// 	// enable signal handlers to save last_rctime
// 	signal(SIGINT, sigint_hdlr);
// 	signal(SIGTERM, sigint_hdlr);
// 	signal(SIGQUIT, sigint_hdlr);
// 	
// 	last_rctime = loadLast_rctime();
// 	
// 	if(!exists(config.sender_dir)) error(SND_DIR_DNE);
// }
// 
// void pollBase(fs::path path, bool loop){
// 	timespec rctime;teensy lc mounting bracket
// 	std::list<fs::path> sync_queue;
// 	Log("Watching: " + path.string(),1);
// 	
// 	do{
// 		auto start = std::chrono::system_clock::now();
// 		if(checkForChange(path, last_rctime, rctime)){
// 			uintmax_t total_bytes = 0;
// 			Log("Change detected in " + path.string(), 1);
// 			// create snapshot
// 			fs::path snapPath = takesnap(rctime);
// 			// wait for rctime to trickle to root
// 			std::this_thread::sleep_for(std::chrono::milliseconds(config.prop_delay_ms));
// 			// launch crawler in snapshot
// 			Log("Launching crawler",2);
// 			crawler(snapPath, sync_queue, snapPath, total_bytes); // enqueue if rctime > last_rctime
// 			// log list of new files
// 			if(config.log_level >= 2){ // skip loop if not logging
// 				Log("Files to sync:",2);
// 				for(auto i : sync_queue){
// 					Log(i.string(),2);
// 				}
// 			}
// 			Log("New files to sync: "+std::to_string(sync_queue.size())+".",1);
// 			// launch rsync
// 			if(!sync_queue.empty()){
// 				launch_procs(sync_queue, config.rsync_nproc, total_bytes);
// 			}
// 			// clear sync queue
// 			sync_queue.clear();
// 			// delete snapshot
// 			deletesnap(snapPath);
// 			// overwrite last_rctime
// 			last_rctime = rctime;
// 		}
// 		auto end = std::chrono::system_clock::now();
// 		std::chrono::duration<double> elapsed = end-start;
// 		if((int)elapsed.count() < config.sync_frequency && loop) // if it took longer than sync freq, don't wait
// 			std::this_thread::sleep_for(std::chrono::seconds(config.sync_frequency - (int)elapsed.count()));
// 	}while(loop);
// 	writeLast_rctime(last_rctime);
// }
// 
// void crawler(fs::path path, std::list<fs::path> &queue, const fs::path &snapdir, uintmax_t &total_bytes){
// 	for(fs::directory_iterator itr{path}; itr != fs::directory_iterator{}; *itr++){
// 		if((config.ignore_hidden == true &&
// 			(*itr).path().filename().string().front() == '.') ||
// 			(config.ignore_win_lock  == true &&
// 			(*itr).path().filename().string().substr(0,2) == "~$") ||
// 			(get_rctime(*itr) < last_rctime))
// 			continue;
// 		if(is_directory(*itr)){
// 			crawler(path/((*itr).path().filename()), queue, snapdir, total_bytes); // recurse
// 		}else{
// 			// cut path at sync dir for rsync /sync_dir/.snap/snapshotX/./rel_path/file
// 			total_bytes += fs::file_size((*itr).path());
// 			queue.emplace_back(snapdir/fs::path(".")/fs::relative((*itr).path(),snapdir));
// 		}
// 	}
// }
// 
// bool checkForChange(const fs::path &path, const timespec &last_rctime, timespec &rctime){
// 	bool change = false;
// 	timespec temp_rctime;
// 	for(fs::directory_iterator itr(path); itr != fs::directory_iterator(); *itr++){
// 		if((temp_rctime = get_rctime(*itr)) > last_rctime){
// 			change = true;
// 			if(temp_rctime > rctime) rctime = temp_rctime; // get highest
// 		}
// 	}
// 	return change;
// }
// 
// fs::path takesnap(const timespec &rctime){
// 	std::string pid = std::to_string(getpid());
// 	fs::path snapPath = fs::path(config.sender_dir).append(".snap/"+pid+"snapshot"+rctime);
// 	Log("Creating snapshot: " + snapPath.string(), 2);
// 	fs::create_directories(snapPath, ec);
// 	if(ec) error(PATH_CREATE, ec);
// 	return snapPath;
// }
// 
// bool deletesnap(fs::path path){
// 	Log("Removing snapshot: " + path.string(), 2);
// 	bool result = fs::remove(path, ec);
// 	if(ec) error(REMOVE_SNAP, ec);
// 	return result;
// }

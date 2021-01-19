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

#include "crawler.hpp"
#include "alert.hpp"
#include <sys/xattr.h>
#include <thread>
#include <chrono>

Crawler::Crawler(const fs::path &config_path, size_t envp_size, const ConfigOverrides &config_overrides)
		: config_(config_path, config_overrides)
		, last_rctime_(config_.last_rctime_path_)
		, syncer(envp_size, config_){
	base_path_ = config_.base_path_;
}

void Crawler::reset(void){
	file_list_.clear();
}

void Crawler::poll_base(bool seed, bool dry_run){
	timespec new_rctime, old_rctime_cache;
	Logging::log.message("Watching: " + base_path_.string(),1);
	if(seed && dry_run) old_rctime_cache = last_rctime_.rctime();
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
				if(!dry_run) syncer.launch_procs(file_list_, total_bytes);
			}
			// clear sync queue
			reset();
			// delete snapshot
			delete_snap(snap_path);
			// overwrite last_rctime
			if(!dry_run) last_rctime_.update(new_rctime);
		}
		auto end = std::chrono::system_clock::now();
		std::chrono::seconds elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
		if(elapsed < config_.sync_period_s_ && !seed && !dry_run) // if it took longer than sync freq, don't wait
			std::this_thread::sleep_for(config_.sync_period_s_ - elapsed);
	}while(!seed && !dry_run);
	if(seed && dry_run) last_rctime_.update(old_rctime_cache);
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
	if(config_.threads_ == 1){ // DFS
		// seed recursive function with snap_path
		find_new_files_recursive(snap_path, snap_path, total_bytes);
	}else if(config_.threads_ > 1){ // multithreaded BFS
		std::atomic<uintmax_t> total_bytes_at(0);
		std::atomic<int> threads_running(0);
		std::vector<std::thread> threads;
		ConcurrentQueue<fs::path> queue;
		// seed list with root node
		queue.push(snap_path);
		// create threads
		for(int i = 0; i < config_.threads_; i++){
			threads.emplace_back(&Crawler::find_new_files_mt_bfs, this, std::ref(queue), snap_path, std::ref(total_bytes_at), std::ref(threads_running));
		}
		for(auto &th : threads) th.join();
		total_bytes = total_bytes_at;
	}else{
		Logging::log.error("Invalid number of worker threads: " + std::to_string(config_.threads_));
	}
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
			file_list_.push_back(snap_root/fs::path(".")/fs::relative(entry.path(), snap_root));
		}
	}
}

void Crawler::find_new_files_mt_bfs(ConcurrentQueue<fs::path> &queue, const fs::path &snap_root, std::atomic<uintmax_t> &total_bytes, std::atomic<int> &threads_running){
	threads_running++;
	{
		std::string msg = "Threads running: " + std::to_string(threads_running);
		Logging::log.message(msg, 2);
	}
	while(!queue.done()){
		fs::path node;
		queue.pop(node, threads_running);
		if(queue.done() && node.string() == "") return;
		{
			std::string msg = "Popped " + node.string();
			Logging::log.message(msg, 2);
		}
		// put children into queue
		if(is_directory(node)){
			for(fs::directory_iterator itr{node}; itr != fs::directory_iterator{}; *itr++){
				fs::directory_entry child = *itr;
				if(ignore_entry(child)) continue;
				std::string msg = "Pushing " + child.path().string();
				Logging::log.message(msg, 2);
				queue.push(child);
			}
		}else{
			total_bytes += fs::file_size(node);
			fs::path formatted_path = snap_root/fs::path(".")/fs::relative(node, snap_root);
			{
				std::unique_lock<std::mutex> lk(file_list_mt_);
				file_list_.push_back(formatted_path);
			}
		}
	}
}

void Crawler::delete_snap(const fs::path &path) const{
	boost::system::error_code ec;
	Logging::log.message("Removing snapshot: " + path.string(), 2);
	fs::remove(path, ec);
	if(ec) Logging::log.error("Error creating path: " + path.string());
}

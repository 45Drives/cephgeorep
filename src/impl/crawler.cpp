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
#include "signal.hpp"
#include <thread>
#include <chrono>

extern "C"{
	#include <sys/xattr.h>
}

Crawler::Crawler(const fs::path &config_path, size_t envp_size, const ConfigOverrides &config_overrides)
		: config_(config_path, config_overrides)
		, last_rctime_(config_.last_rctime_path_)
		, syncer(envp_size, config_){
	base_path_ = config_.base_path_;
	set_signal_handlers(this);
}

void Crawler::poll_base(bool seed, bool dry_run, bool set_rctime, bool oneshot){
	timespec new_rctime = {0};
	timespec old_rctime_cache = {0};
	std::chrono::steady_clock::duration last_rctime_flush_period = std::chrono::hours(1);
	std::chrono::steady_clock::time_point last_rctime_last_flush = std::chrono::steady_clock::now();
	Logging::log.message("Watching: " + base_path_.string(),1);
	if(seed && dry_run) old_rctime_cache = last_rctime_.rctime();
	if(seed) last_rctime_.update({1}); // sync everything
	do{
		auto start = std::chrono::steady_clock::now();
		Logging::log.message("Checking for change.", 2);
		if(last_rctime_.check_for_change(base_path_, new_rctime)){
			Logging::log.message("Change detected in " + base_path_.string(), 1);
			std::vector<File> file_list;
			uintmax_t total_bytes = 0;
			// take snapshot
			create_snap(new_rctime);
			// wait for rctime to trickle to root
			std::this_thread::sleep_for(config_.prop_delay_ms_);
			// queue files
			trigger_search(file_list, snap_path_, total_bytes);
			if(!set_rctime){
				std::string msg = "New files to sync: " + std::to_string(file_list.size());
				msg += " (" + Logging::log.format_bytes(total_bytes) + ")";
				Logging::log.message(msg, 1);
			}
			// launch rsync
			if(!file_list.empty()){
				if(dry_run){
					std::string msg = config_.exec_bin_ + " " + config_.exec_flags_ + " <file list> ";
					msg += syncer.construct_destination(config_.remote_user_, config_.remote_host_, config_.remote_directory_);
					Logging::log.message(msg, 1);
				}else if(!set_rctime){
					syncer.sync(file_list);
				}
			}
			// delete snapshot
			delete_snap();
			// overwrite last_rctime
			if(!dry_run){
				last_rctime_.update(new_rctime);
				auto now = std::chrono::steady_clock::now();
				if(now - last_rctime_last_flush >= last_rctime_flush_period){
					last_rctime_.write_last_rctime();
					last_rctime_last_flush = now;
				}
			}
			file_list.clear();
			file_list = std::vector<File>(); // try to free memory taken by vector
		}
		if(oneshot)
			break;
		auto end = std::chrono::steady_clock::now();
		std::chrono::seconds elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
		if(elapsed < config_.sync_period_s_ && !seed && !dry_run && !set_rctime) // if it took longer than sync freq, don't wait
			std::this_thread::sleep_for(config_.sync_period_s_ - elapsed);
	}while(!seed && !dry_run && !set_rctime);
	if(seed && dry_run) last_rctime_.update(old_rctime_cache);
}

void Crawler::create_snap(const timespec &rctime){
	boost::system::error_code ec;
	std::string pid = std::to_string(getpid());
	snap_path_ = fs::path(base_path_).append(".snap/"+pid+"snapshot"+rctime);
	Logging::log.message("Creating snapshot: " + snap_path_.string(), 2);
	fs::create_directories(snap_path_, ec);
	if(ec){
		Logging::log.error("Error creating snapshot path: " + ec.message());
		l::exit(EXIT_FAILURE);
	}
}

void Crawler::trigger_search(std::vector<File> &file_list, const fs::path &snap_path, uintmax_t &total_bytes){
	// launch crawler in snapshot
	Logging::log.message("Launching crawler",2);
	if(config_.threads_ == 1){ // DFS
		// seed recursive function with snap_path
		find_new_files_recursive(file_list, snap_path, snap_path, total_bytes);
	}else if(config_.threads_ > 1){ // multithreaded BFS
		std::atomic<uintmax_t> total_bytes_at(0);
		std::atomic<int> threads_running(0);
		std::vector<std::thread> threads;
		ConcurrentQueue<fs::path> queue;
		// seed list with root node
		queue.push(snap_path);
		// create threads
		for(int i = 0; i < config_.threads_; i++){
			threads.emplace_back(&Crawler::find_new_files_mt_bfs, this, std::ref(file_list), std::ref(queue), snap_path, std::ref(total_bytes_at), std::ref(threads_running));
		}
		for(auto &th : threads) th.join();
		total_bytes = total_bytes_at;
	}else{
		Logging::log.error("Invalid number of worker threads: " + std::to_string(config_.threads_));
		l::exit(EXIT_FAILURE);
	}
	// log list of new files
	if(config_.log_level_ >= 2){ // skip loop if not logging
		Logging::log.message("Files to sync:",2);
		for(auto &i : file_list){
			Logging::log.message(i.path(),2);
		}
	}
}

inline const char *get_last_of(const char *string, char test){
	const char *last = nullptr;
	while(*string++ != '\0')
		if(*string == test)
			last = string;
	return last;
}

inline const char *get_filename(const char *path){
	const char *file_name =  get_last_of(path, '/');
	if(file_name == nullptr)
		return nullptr;
	return file_name+1;
}

inline bool check_hidden(const char *file_name){
	// /^\./
	return *file_name == '.';
}

inline bool check_win_lock(const char *file_name){
	// /^~\$/
	return (file_name[0] == '~' && file_name[1] == '$');
}

inline bool check_vim_swap(const char *file_name){
	// /^\..*\.swpx?$/
	// move to end
	if(!check_hidden(file_name))
		return false;
	const char *ext = get_last_of(file_name, '.');
	if(ext == nullptr)
		return false;
	return (
		   ext[1] == 's'
		&& ext[2] == 'w'
		&& ext[3] == 'p'
		&& (
			   ext[4] == '\0' /* ends in .swp */
			|| (
				   ext[4] == 'x'
				&& ext[5] == '\0' /* ends in .swpx */
			)
		)
	);
}

bool Crawler::ignore_entry(const File &file) const{
	if(last_rctime_.is_newer(file)){
		const char *file_name = get_filename(file.path());
		return ( // returns true if any of the following tests return true, false if all are false
			    (config_.ignore_hidden_ && check_hidden(file_name))
			||  (config_.ignore_win_lock_ && check_win_lock(file_name))
			||  (config_.ignore_vim_swap_ && check_vim_swap(file_name))
		);
	}else{
		return true; // ignore if older than current last_rctime_
	}
}

void Crawler::find_new_files_recursive(std::vector<File> &file_list, fs::path current_path, const fs::path &snap_root, uintmax_t &total_bytes){
	size_t snap_root_len = snap_root.string().length();
	for(fs::directory_iterator itr{current_path}; itr != fs::directory_iterator{}; *itr++){
		fs::directory_entry entry = *itr;
		const char *path = entry.path().c_str();
		File file(path, snap_root_len);
		if(ignore_entry(file)) continue;
		if(file.is_directory()){
			find_new_files_recursive(file_list, entry.path(), snap_root, total_bytes); // recurse
		}else{
			total_bytes += file.size();
			file_list.emplace_back(std::move(file));
		}
	}
}

void Crawler::find_new_files_mt_bfs(std::vector<File> &file_list, ConcurrentQueue<fs::path> &queue, const fs::path &snap_root, std::atomic<uintmax_t> &total_bytes, std::atomic<int> &threads_running){
	threads_running++;
	bool nodes_left = true;
	fs::path node;
	std::vector<File> files_to_enqueue;
	files_to_enqueue.reserve(128);
	size_t snap_root_len = snap_root.string().length();
	while(nodes_left){
		nodes_left = queue.pop(node, threads_running);
		if(!nodes_left) return;
		// put all child directories back in queue
		for(fs::directory_iterator itr{node}; itr != fs::directory_iterator{}; *itr++){
			File file(itr->path().c_str(), snap_root_len);
			if(!ignore_entry(file)){
				if(file.is_directory()){
					// put child directory into queue
					queue.push(*itr);
				}else{
					// save non-directory children in temp queue
					files_to_enqueue.emplace_back(std::move(file));
				}
			}
		}
		for(File &file : files_to_enqueue){
			total_bytes += file.size();
			{
				std::unique_lock<std::mutex> lk(file_list_mt_);
				file_list.emplace_back(std::move(file));
			}
		}
		files_to_enqueue.clear();
	}
}

void Crawler::delete_snap(void) const{
	boost::system::error_code ec;
	Logging::log.message("Removing snapshot: " + snap_path_.string(), 2);
	fs::remove(snap_path_, ec);
	if(ec){
		Logging::log.error("Error removing snapshot path: " + ec.message());
		exit(EXIT_FAILURE);
	}
}

void Crawler::write_last_rctime(void) const{
	last_rctime_.write_last_rctime();
}

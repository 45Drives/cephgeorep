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

//#define DEBUG_BATCHES

#include "exec.hpp"
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>

#ifdef DEBUG_BATCHES
#include <iostream>
#endif

//#define MAX_SZ_LIM 3*(8*1024*1024)/4

/* SyncProcess --------------------------------
 */

SyncProcess::SyncProcess(Syncer *parent, uintmax_t max_bytes_sz)
    : exec_bin_(parent->exec_bin_), exec_flags_(parent->exec_flags_), destination_(parent->destination_){
	id_ = -1;
	curr_bytes_sz_ = 0;
	max_arg_sz_ = parent->max_arg_sz_;
	start_arg_sz_= curr_arg_sz_ = parent->start_arg_sz_;
	max_bytes_sz_ = max_bytes_sz;
}

SyncProcess::~SyncProcess(){
	for(char *i : garbage_){
		delete [] i;
	} 
}

int SyncProcess::id() const{
	return id_;
}

pid_t SyncProcess::pid() const{
	return pid_;
}

void SyncProcess::set_id(int id){
	id_ = id;
}

uintmax_t SyncProcess::payload_sz(void) const{
	return curr_bytes_sz_;
}

uintmax_t SyncProcess::payload_count(void) const{
	return files_.size();
}

void SyncProcess::add(const fs::path &file){
	files_.emplace_back(file);
	curr_arg_sz_ += strlen(file.c_str()) + 1 + sizeof(char *);
	curr_bytes_sz_ += fs::file_size(file);
}

bool SyncProcess::full_test(const fs::path &file) const{
	return (curr_arg_sz_ + strlen(file.c_str()) + 1 + sizeof(char *) >= max_arg_sz_)
	|| (curr_bytes_sz_ + fs::file_size(file) > max_bytes_sz_);
}

bool SyncProcess::large_file(const fs::path &file) const{
	return curr_bytes_sz_ < max_bytes_sz_ && fs::file_size(file) >= max_bytes_sz_;
}

void SyncProcess::consume(std::list<fs::path> &queue){
	while(!queue.empty() && (!full_test(queue.front()) || large_file(queue.front()))){
		add(fs::path(queue.front()));
		queue.pop_front();
	}
}

void SyncProcess::consume_one(std::list<fs::path> &queue){
	add(fs::path(queue.front()));
	queue.pop_front();
}

void SyncProcess::sync_batch(){
	std::vector<char *> argv;
	
	argv.push_back((char *)exec_bin_.c_str());
	
	// push back flags
	boost::tokenizer<boost::escaped_list_separator<char>> tokens(
		exec_flags_,
		boost::escaped_list_separator<char>(
			std::string("\\"), std::string(" "), std::string("\"\'")
		)
	);
	for(
		boost::tokenizer<boost::escaped_list_separator<char>>::iterator itr = tokens.begin();
		itr != tokens.end();
		++itr
	){
		char *flag = new char[(*itr).length()+1];
		strcpy(flag,(*itr).c_str());
		argv.push_back(flag);
		garbage_.push_back(flag);
	}
	
	// for each file of batch
	for(std::vector<fs::path>::iterator fitr = files_.begin(); fitr != files_.end(); ++fitr){
		char *path = new char[fitr->string().length()+1];
		std::strcpy(path,fitr->c_str());
		argv.push_back(path);
		garbage_.push_back(path); // for cleanup
	}
	
	if(!destination_->empty()) argv.push_back((char *)destination_->c_str());
	argv.push_back(NULL);
	
	pid_ = fork(); // create child process
	switch(pid_){
		case -1:
			Logging::log.error("Forking failed");
			break;
		case 0: // child process
			execvp(argv[0],&argv[0]);
			Logging::log.error("Failed to execute " + exec_bin_);
			break;
		default: // parent process
			Logging::log.message(std::to_string(pid_) + " started.", 2);
			break;
	}
}

void SyncProcess::clear_file_list(void){
	files_.clear();
	curr_bytes_sz_ = 0;
	curr_arg_sz_ = start_arg_sz_;
}

/* Syncer --------------------------------
 */

Syncer::Syncer(size_t envp_size, const Config &config)
    : exec_bin_(config.exec_bin_), exec_flags_(config.exec_flags_){
	nproc_ = config.nproc_;
	max_arg_sz_ = get_max_arg_sz();
	start_arg_sz_ = envp_size
				+ exec_bin_.length() + 1// length of executable name
				+ exec_flags_.length() + 1 // length of flags
				+ sizeof(char *) * 2 // size of char pointers
				+ sizeof(NULL);
	boost::tokenizer<boost::escaped_list_separator<char>> tokens(
		config.destinations_,
		boost::escaped_list_separator<char>(
			std::string("\\"), std::string(", "), std::string("\"\'")
		)
	);
	for(
		boost::tokenizer<boost::escaped_list_separator<char>>::iterator itr = tokens.begin();
		itr != tokens.end();
		++itr
	){
		destinations_.emplace_back(*itr);
	}
	if(destinations_.empty())
		destinations_.push_back(construct_destination(config.remote_user_, config.remote_host_, config.remote_directory_));
	destination_ = destinations_.begin();
	if(!destination_->empty()){
		start_arg_sz_ += destination_->length() + 1 + sizeof(char *);
	}
}

std::string Syncer::construct_destination(std::string remote_user, std::string remote_host, fs::path remote_directory) const{
	std::string dest = remote_directory.string();
	if(!remote_host.empty()){
		dest = remote_host + ":" + dest;
		if(!remote_user.empty()){
			dest = remote_user + "@" + dest;
		}
	}
	return dest;
}

size_t Syncer::get_max_arg_sz(void) const{
	struct rlimit lims;
	getrlimit(RLIMIT_STACK, &lims);
	size_t arg_max_sz = lims.rlim_cur / 4;
	// if(arg_max_sz > MAX_SZ_LIM) arg_max_sz = MAX_SZ_LIM;
	arg_max_sz -= 4*2048; // allow 2048 bytes of headroom
	return arg_max_sz;
}

void Syncer::launch_procs(std::list<fs::path> &queue, uintmax_t total_bytes){
	int wstatus;
	
	// cap nproc to between 1 and number of files
	int nproc = std::min(nproc_, (int)queue.size());
	nproc = std::max(nproc, 1);
	
	uintmax_t bytes_per_proc = total_bytes / nproc + (total_bytes % nproc != 0);
	Logging::log.message(std::to_string(bytes_per_proc) + " bytes per proc", 2);
	
	std::list<SyncProcess> procs(nproc, {this, bytes_per_proc});
	
	// sort files from largest to smallest to get largest files out of the way first
	queue.sort([](const fs::path &first, const fs::path &second){
		return fs::file_size(first) > fs::file_size(second);
	});
	
	// round-robin distribute until procs are full to total_bytes/nproc
	distribute_files(queue, procs);
	
	// start each process
	int proc_id = 0; // incremental ID for each process
	for(std::list<SyncProcess>::iterator proc_itr = procs.begin(); proc_itr != procs.end(); ++proc_itr){
		{
			std::string msg = "Launching " + exec_bin_ + " " + exec_flags_ + " with " + std::to_string(proc_itr->payload_count()) + " files.";
			if(nproc > 1) msg = "Proc " + std::to_string(proc_id) + ": " + msg;
			Logging::log.message(msg, 1);
		}
		proc_itr->set_id(proc_id++);
		Logging::log.message(std::to_string(proc_itr->payload_sz()) + " bytes", 2);
		proc_itr->sync_batch();
	}
	int num_ssh_fails = 0;
	while(!procs.empty()){ // while files are remaining in batch queues
		// wait for a child to change state then relaunch remaining batches
		pid_t exited_pid = wait(&wstatus);
		// find which object PID belongs to
		std::list<SyncProcess>::iterator exited_proc = std::find_if (
			procs.begin(), procs.end(),
			[&exited_pid](const SyncProcess &proc){
				return proc.pid() == exited_pid;
			}
		);
		// check exit code
		switch(WEXITSTATUS(wstatus)){
			case SUCCESS:
				Logging::log.message(std::to_string(exited_pid) + " exited successfully.",2);
				exited_proc->clear_file_list(); // remove synced batch
				num_ssh_fails = 0;
				break;
			case SSH_FAIL:
				Logging::log.warning(exec_bin_ + " failed to connect to" + *destination_ + "\n"
				"Is the server running and connected to your network?");
				if(std::next(destination_) == destinations_.end())
					Logging::log.error("No more beackup destinations to try.");
				if((num_ssh_fails = ++num_ssh_fails % nproc) == 0)
					++destination_; // increment destination itr if all procs fail
				break;
			case NOT_INSTALLED:
				Logging::log.error(exec_bin_ + " is not installed.");
				break;
			case PERM_DENY:
				Logging::log.error("Encountered permission error while executing " + exec_bin_);
				break;
			default:
				Logging::log.error("Encountered unknown error while executing " + exec_bin_);
				break;
		}
		if(queue.empty() && exited_proc->payload_sz() == 0){
			{
				std::string msg = "done.";
				if(nproc > 1) msg = "Proc " + std::to_string(exited_proc->id()) + ": " + msg;
				Logging::log.message(msg, 1);
			}
			procs.erase(exited_proc);
		}else{
			if(!queue.empty()) exited_proc->consume(queue);
			{
				std::string msg = "Launching " + exec_bin_ + " " + exec_flags_ + " with " + std::to_string(exited_proc->payload_count()) + " files.";
				if(nproc > 1) msg = "Proc " + std::to_string(exited_proc->id()) + ": " + msg;
				Logging::log.message(msg, 1);
			}
			exited_proc->sync_batch();
		}
	}
}

void Syncer::distribute_files(std::list<fs::path> &queue, std::list<SyncProcess> &procs) const{
	// create roster for round robin distribution
	std::list<SyncProcess *> distribute_pool;
	for(std::list<SyncProcess>::iterator itr = procs.begin(); itr != procs.end(); ++itr){
		distribute_pool.emplace_back(&(*itr));
	}
	// deal out files until either all procs are full to total_bytes/nproc or queue is empty
	std::list<SyncProcess *>::iterator proc_itr = distribute_pool.begin();
	while(!queue.empty() && !distribute_pool.empty()){
		if((*proc_itr)->full_test(queue.front()) && !(*proc_itr)->large_file(queue.front())){
			// number of bytes > total_bytes/nproc so remove proc from roster
			proc_itr = distribute_pool.erase(proc_itr);
		}else{
			(*proc_itr)->consume_one(queue);
			++proc_itr;
		}
		// circularly iterate
		if(proc_itr == distribute_pool.end()) proc_itr = distribute_pool.begin();
	}
}

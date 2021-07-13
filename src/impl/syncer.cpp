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

#include "syncer.hpp"
#include "syncProcess.hpp"
#include "config.hpp"
#include "file.hpp"
#include "alert.hpp"
#include "signal.hpp"
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <chrono>
#include <thread>

#ifndef NO_PARALLEL_SORT
#include <execution>
#endif

extern "C" {
	#include <unistd.h>
	#include <sys/wait.h>
	#include <sys/resource.h>
	#include <fcntl.h>
	#include <string.h>
	#include <limits.h>
}

Syncer::Syncer(size_t envp_size, const Config &config)
    : exec_bin_(config.exec_bin_), exec_flags_(config.exec_flags_){
	nproc_ = config.nproc_;
	max_mem_usage_ = get_mem_limit(envp_size);
	
	start_mem_usage_ = 0;
	
	start_payload_.push_back((char *)exec_bin_.c_str());
	start_mem_usage_ += exec_bin_.length() + 1 + sizeof(char *); // length of executable name
	
	// account for flags
	{
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
			// push back flags
			char *flag = new char[(*itr).length()+1];
			strcpy(flag, itr->c_str());
			start_payload_.push_back(flag);
			garbage_.push_back(flag);
			// account for their size
			start_mem_usage_ += itr->length() + 1 + sizeof(char *);
		}
	}
	
	{
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
			if(itr->empty())
				continue;
			destinations_.emplace_back(*itr);
		}
	}
	
	if(destinations_.empty())
		destinations_.push_back(construct_destination(config.remote_user_, config.remote_host_, config.remote_directory_));
	
	// account for destinations
	int max_destination_len = 0;
	for(const std::string &dest : destinations_){
		int test_len = dest.length();
		if(test_len > max_destination_len)
			max_destination_len = test_len;
	}
	destination_ = destinations_.begin();
	start_mem_usage_ += max_destination_len + 1 + sizeof(char *);
	
	start_mem_usage_ += sizeof(NULL);
}

Syncer::~Syncer(){
	for(char *string : garbage_)
		delete[] string;
}

std::string Syncer::construct_destination(std::string remote_user, std::string remote_host, std::string remote_directory) const{
	std::string dest = remote_directory;
	if(!remote_host.empty()){
		dest = remote_host + ":" + dest;
		if(!remote_user.empty()){
			dest = remote_user + "@" + dest;
		}
	}
	return dest;
}

size_t Syncer::get_mem_limit(size_t envp_size) const{
	long arg_max = sysconf(_SC_ARG_MAX);
	if(arg_max == -1){
		int err = errno;
		Logging::log.warning(std::string("Could not determine ARG_MAX from syscall: ") + strerror(err));
		arg_max = _POSIX_ARG_MAX;
	}
	return arg_max - envp_size - MEM_LIM_HEADROOM;
}

void Syncer::sync(std::vector<File> &queue){
	std::list<SyncProcess> procs;

	// sort files from smallest to largest to get largest files out of the way first from end
	std::sort(
#ifndef NO_PARALLEL_SORT
		std::execution::par,
#endif
		queue.begin(), queue.end(),
		[](const File &first, const File &second){
		return first.size() < second.size();
	});

	LAUNCH_PROCS_RET_T res;
	do{
		launch_procs(procs, queue);
		res = handle_returned_procs(procs, queue);
		if(res == INC_HEADROOM){
			if(max_mem_usage_ > MEM_LIM_HEADROOM*2)
				max_mem_usage_ -= MEM_LIM_HEADROOM;
			else{
				Logging::log.error("Ran out of room while increasing memory headroom.");
				res = SYNC_FAILED;
			}
		}
	}while(res != SYNC_SUCCESS && res != SYNC_FAILED);
	if(res == SYNC_FAILED)
		l::exit(EXIT_FAILURE);
}

void Syncer::launch_procs(std::list<SyncProcess> &procs, std::vector<File> &queue){
	// cap nproc to between 1 and number of files
	int nproc = std::min(nproc_, (int)queue.size());
	nproc = std::max(nproc, 1);

	for(int i = 0; i < nproc; i++){
		procs.emplace_back(this, i, nproc, queue);
	}
	
	// start each process
	for(SyncProcess &proc : procs){
		proc.consume(queue);
		std::string msg = "Launching " + exec_bin_ + " " + exec_flags_ + " with " + std::to_string(proc.payload_count()) + " files.";
		if(nproc > 1) msg = "Proc " + std::to_string(proc.id()) + ": " + msg;
		Logging::log.message(msg, 1);
		proc.sync_batch();
	}
}

static inline bool ends_with(const std::string& str, const std::string& suffix){
	return str.size() >= suffix.size()
		&& str.compare(str.size()-suffix.size(), suffix.size(), suffix) == 0;
}

LAUNCH_PROCS_RET_T Syncer::handle_returned_procs(std::list<SyncProcess> &procs, std::vector<File> &queue){
	int wstatus;
	const unsigned int num_ssh_fails_to_inc = procs.size(); // increment destination_ when ssh fails and this is 0
	int nproc = procs.size();
	std::vector<SyncProcess *> ssh_fail_procs; // hold on to procs that fail from SSH error
	while(!procs.empty()){ // while files are remaining in batch queues
		// wait for a child to change state then relaunch remaining batches
		pid_t exited_pid = wait(&wstatus);
		if(exited_pid == -1){
			Logging::log.error("No children to wait for");
			return SYNC_FAILED;
		}
		// find which object PID belongs to
		std::list<SyncProcess>::iterator exited_proc = std::find_if (
			procs.begin(), procs.end(),
			[&exited_pid](const SyncProcess &proc){
				return proc.pid() == exited_pid;
			}
		);
		if(exited_proc == procs.end())
			continue;
		// check exit code
		int exit_code = WEXITSTATUS(wstatus);

		if(exit_code == 0){ // success
			Logging::log.message(std::to_string(exited_pid) + " exited successfully.",2);
			Status::status.set(Status::OK);
			exited_proc->reset();
			if(exited_proc->done(queue)){
				{
					std::string msg = "done.";
					if(nproc > 1) msg = "Proc " + std::to_string(exited_proc->id()) + ": " + msg;
					Logging::log.message(msg, 1);
				}
				procs.erase(exited_proc);
			}else{
				exited_proc->consume(queue);
				{
					std::string msg = "Launching " + exec_bin_ + " " + exec_flags_ + " with " + std::to_string(exited_proc->payload_count()) + " files.";
					if(nproc > 1) msg = "Proc " + std::to_string(exited_proc->id()) + ": " + msg;
					Logging::log.message(msg, 1);
				}
				exited_proc->sync_batch();
			}
			for(SyncProcess *proc_ptr : ssh_fail_procs){
				{
					std::string msg = "Retrying since others succeeded.";
					if(nproc > 1) msg = "Proc " + std::to_string(proc_ptr->id()) + ": " + msg;
					Logging::log.message(msg, 1);
				}
				proc_ptr->sync_batch();
			}
			ssh_fail_procs.clear();
		}else if(exit_code == CHECK_SHMEM && exited_proc->exec_error_ && exited_proc->exec_error_->exec_failed_){
			int returned_errno = exited_proc->exec_error_->errno_;
			exited_proc->dump_argv(returned_errno);
			char *errno_msg = strerror(returned_errno);
			if(errno_msg){
				Logging::log.error("proc " + std::to_string(exited_proc->id()) + ": Failed to execute " + exec_bin_ + ": " + errno_msg);
				if(returned_errno == E2BIG){
					Logging::log.message("Changing ARG_MAX headroom and trying again.", 1);
					for(const SyncProcess &proc : procs){
						if(proc.pid() == 0 || exited_pid == proc.pid())
							continue;
						kill(proc.pid(), SIGINT);
					}
					// wait for all children to exit
					while((exited_pid = wait(NULL)) != -1){}
					procs.clear();
					return INC_HEADROOM;
				}
			}else{
				Logging::log.error(
					"Encountered unknown error while executing " + exec_bin_ + "\n"
					"Exit code: " + std::to_string(returned_errno)
				);
			}
			return SYNC_FAILED;
		}else{ // rsync error
			std::string error_message = exited_proc->log_errors();
			if(exit_code == PROTOCOL_STREAM && ends_with(exec_bin_, "rsync") && error_message.find("command not found") != std::string::npos){
				Logging::log.error("Destination does not have rsync installed.");
				return SYNC_FAILED;
			}
			switch(exit_code){
			case PROTOCOL_STREAM:
			case PARTIAL_XFR:
			case TIMEOUT_S_R:
			case TIMEOUT_CONN:
				if(ends_with(exec_bin_, "rsync")){
					Logging::log.error(Logging::log.rsync_error(exit_code));
					Logging::log.message("Trying batch again.", 1);
					exited_proc->sync_batch();
					break;
				}
				Logging::log.error("Unknown exit code from " + exec_bin_ + ": " + std::to_string(exit_code));
				return SYNC_FAILED;
			case SSH_FAIL:
				{
					std::string msg = exec_bin_ + " failed to connect to " + exited_proc->destination() + ". Is the server running and connected to your network?";
					if(nproc > 1) msg = "Proc " + std::to_string(exited_proc->id()) + ": " + msg;
					Logging::log.warning(msg);
				}
				ssh_fail_procs.push_back(&(*exited_proc));
				if(ssh_fail_procs.size() >= num_ssh_fails_to_inc){
					Status::status.set(Status::HOST_DOWN);
					if(++destination_ == destinations_.end()){ // increment destination itr if all procs fail
						destination_ = destinations_.begin();
						Logging::log.message("Waiting for 30 seconds before trying first destination again.", 1);
						std::this_thread::sleep_for(std::chrono::seconds(30));
						Logging::log.message("Trying first destination again.", 1);
					}else{
						Logging::log.message("Trying next destination.", 1);
					}
					ssh_fail_procs.clear();
					// restart all procs
					for(SyncProcess &proc : procs){
						proc.change_destination();
						proc.sync_batch();
					}
				}
				break;
			default:
				Logging::log.error("Unknown exit code from " + exec_bin_ + ": " + std::to_string(exit_code));
				if(ends_with(exec_bin_, "rsync")){
					Logging::log.error(Logging::log.rsync_error(exit_code));
				}
				return SYNC_FAILED;
			}
		}
	}
	return SYNC_SUCCESS;
}

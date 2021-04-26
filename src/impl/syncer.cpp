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

#ifndef NO_PARALLEL_SORT
#include <execution>
#endif


extern "C" {
	#include <unistd.h>
	#include <sys/wait.h>
	#include <sys/resource.h>
	#include <fcntl.h>
	#include <string.h>
}

Syncer::Syncer(size_t envp_size, const Config &config)
    : exec_bin_(config.exec_bin_), exec_flags_(config.exec_flags_){
	nproc_ = config.nproc_;
	max_mem_usage_ = get_mem_limit();
	start_mem_usage_ = envp_size
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
		if(itr->empty())
			continue;
		destinations_.emplace_back(*itr);
	}
	if(destinations_.empty())
		destinations_.push_back(construct_destination(config.remote_user_, config.remote_host_, config.remote_directory_));
	destination_ = destinations_.begin();
	if(!destination_->empty()){
		start_mem_usage_ += destination_->length() + 1 + sizeof(char *);
	}
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

size_t Syncer::get_mem_limit(void) const{
	struct rlimit lims;
	getrlimit(RLIMIT_STACK, &lims);
	size_t arg_max_sz = lims.rlim_cur / 4;
	arg_max_sz -= 2048; // allow 2048 bytes of headroom
	long arg_max = sysconf(_SC_ARG_MAX);
	if(arg_max == -1){
		int err = errno;
		Logging::log.warning(std::string("Could not determine ARG_MAX from syscall: ") + strerror(err));
	}else{
		if(arg_max_sz > arg_max) arg_max_sz = arg_max;
	}
	return arg_max_sz;
}

void Syncer::launch_procs(std::vector<File> &queue, uintmax_t total_bytes){
	int wstatus;
	
	// cap nproc to between 1 and number of files
	int nproc = std::min(nproc_, (int)queue.size());
	nproc = std::max(nproc, 1);
	
	// sort files from smallest to largest to get largest files out of the way first from end
	std::sort(
#ifndef NO_PARALLEL_SORT
		std::execution::par,
#endif
		queue.begin(), queue.end(),
		[](const File &first, const File &second){
		return first.size() < second.size();
	});
	
	std::list<SyncProcess> procs;
	
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
	int num_ssh_fails_to_inc = 0; // increment destination_ when ssh fails and this is 0
	while(!procs.empty()){ // while files are remaining in batch queues
		// wait for a child to change state then relaunch remaining batches
		pid_t exited_pid = wait(&wstatus);
		if(exited_pid == -1){
			Logging::log.error("No children to wait for");
			l::exit(EXIT_FAILURE);
		}else{
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
					exited_proc->reset();
					if(!exited_proc->done(queue))
						exited_proc->consume(queue);
					break;
				case SSH_FAIL:
					{
						std::string msg = exec_bin_ + " failed to connect to " + exited_proc->destination() + ". Is the server running and connected to your network?";
						if(nproc > 1) msg = "Proc " + std::to_string(exited_proc->id()) + ": " + msg;
						Logging::log.warning(msg);
					}
					if(num_ssh_fails_to_inc == 0){
						if(std::next(destination_) == destinations_.end()){
							Logging::log.error("No more backup destinations to try.");
							l::exit(EXIT_FAILURE, Status::ALL_HOSTS_DOWN);
						}
						Logging::log.message("Trying next destination", 1);
						Status::status.set(Status::HOST_DOWN);
						++destination_; // increment destination itr if all procs fail
						num_ssh_fails_to_inc = nproc - 1;
					}else{
						--num_ssh_fails_to_inc;
					}
					break;
				case NOT_INSTALLED:
					Logging::log.error(exec_bin_ + " is not installed.");
					l::exit(EXIT_FAILURE);
				case PERM_DENY:
					Logging::log.error("Encountered permission error while executing " + exec_bin_);
					l::exit(EXIT_FAILURE);
				default:
					int8_t status = WEXITSTATUS(wstatus);
					if(status > 0){
						Logging::log.error(
							"Encountered error while executing " + exec_bin_ + "\n"
							"Exit code: " + std::to_string(status)
						);
						if(exec_bin_.find("rsync") != std::string::npos)
							Logging::log.error("rsync exit status: " + Logging::log.rsync_error(status));
						l::exit(EXIT_FAILURE);
					}else{
						char *errno_msg = strerror(-status);
						if(errno_msg){
							Logging::log.error("Failed to execute " + exec_bin_ + ": " + errno_msg);
						}else{
							Logging::log.error(
								"Encountered unknown error while executing " + exec_bin_ + "\n"
								"Exit code: " + std::to_string(status)
							);
						}
						l::exit(EXIT_FAILURE);
					}
					break;
			}
			if(exited_proc->done(queue)){
				{
					std::string msg = "done.";
					if(nproc > 1) msg = "Proc " + std::to_string(exited_proc->id()) + ": " + msg;
					Logging::log.message(msg, 1);
				}
				procs.erase(exited_proc);
			}else{
				{
					std::string msg = "Launching " + exec_bin_ + " " + exec_flags_ + " with " + std::to_string(exited_proc->payload_count()) + " files.";
					if(nproc > 1) msg = "Proc " + std::to_string(exited_proc->id()) + ": " + msg;
					Logging::log.message(msg, 1);
				}
				exited_proc->sync_batch();
			}
		}
	}
}

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

#include "exec.hpp"
#include "signal.hpp"
#include "status.hpp"
#include <algorithm>

#ifndef NO_PARALLEL_SORT
#include <execution>
#endif

#include <boost/tokenizer.hpp>

extern "C" {
	#include <unistd.h>
	#include <sys/wait.h>
	#include <sys/resource.h>
	#include <fcntl.h>
	#include <string.h>
}

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

void SyncProcess::add(const File &file){
	files_.emplace_back(file.path());
	curr_arg_sz_ += strlen(file.path().c_str()) + 1 + sizeof(char *);
	if(!fs::is_symlink(file.status()))
		curr_bytes_sz_ += file.size();
}

bool SyncProcess::full_test(const File &file) const{
	if(fs::is_symlink(file.status()))
		return (curr_arg_sz_ + strlen(file.path().c_str()) + 1 + sizeof(char *) >= max_arg_sz_);
	return (curr_arg_sz_ + strlen(file.path().c_str()) + 1 + sizeof(char *) >= max_arg_sz_)
	|| (curr_bytes_sz_ + file.size() > max_bytes_sz_);
}

bool SyncProcess::large_file(const File &file) const{
	return curr_bytes_sz_ < max_bytes_sz_ && file.size() >= max_bytes_sz_;
}

void SyncProcess::consume(std::vector<File> &queue){
	while(!queue.empty() && (!full_test(queue.front().path()) || large_file(queue.front().path()))){
		consume_one(queue);
	}
}

void SyncProcess::consume_one(std::vector<File> &queue){
	add(queue.back().path());
	queue.pop_back();
}

void SyncProcess::sync_batch(){
	sending_to_ = *destination_;
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
	int error;
	switch(pid_){
		case -1:
			error = errno;
			Logging::log.error(std::string("Forking failed: ") + strerror(error));
			l::exit(EXIT_FAILURE);
		case 0: // child process
			{
				int null_fd = open("/dev/null", O_WRONLY);
				dup2(null_fd, 1);
				dup2(null_fd, 2);
				close(null_fd);
				execvp(argv[0],&argv[0]);
				int execvp_errno = -errno;
				exit(execvp_errno);
			}
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

bool SyncProcess::payload_empty(void) const{
	return files_.empty();
}

const std::string &SyncProcess::destination(void) const{
	return sending_to_;
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
		if(itr->empty())
			continue;
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

void Syncer::launch_procs(std::vector<File> &queue, uintmax_t total_bytes){
	int wstatus;
	
	// cap nproc to between 1 and number of files
	int nproc = std::min(nproc_, (int)queue.size());
	nproc = std::max(nproc, 1);
	
	uintmax_t bytes_per_proc = total_bytes / nproc + (total_bytes % nproc != 0);
	Logging::log.message(std::to_string(bytes_per_proc) + " bytes per proc", 2);
	
	std::list<SyncProcess> procs(nproc, {this, bytes_per_proc});
	
	// sort files from smallest to largest to get largest files out of the way first from end
	std::sort(
#ifndef NO_PARALLEL_SORT
		std::execution::par,
#endif
		queue.begin(), queue.end(),
		[](const File &first, const File &second){
		return first.size() < second.size();
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
					exited_proc->clear_file_list(); // remove synced batch
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
			if(queue.empty() && exited_proc->payload_empty()){
				{
					std::string msg = "done.";
					if(nproc > 1) msg = "Proc " + std::to_string(exited_proc->id()) + ": " + msg;
					Logging::log.message(msg, 1);
				}
				procs.erase(exited_proc);
			}else if(exited_proc->payload_empty()){
				if(!queue.empty()) exited_proc->consume(queue);
				{
					std::string msg = "Launching " + exec_bin_ + " " + exec_flags_ + " with " + std::to_string(exited_proc->payload_count()) + " files.";
					if(nproc > 1) msg = "Proc " + std::to_string(exited_proc->id()) + ": " + msg;
					Logging::log.message(msg, 1);
				}
				exited_proc->sync_batch();
			}else{
				exited_proc->sync_batch();
			}
		}
	}
}

void Syncer::distribute_files(std::vector<File> &queue, std::list<SyncProcess> &procs) const{
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
	queue.shrink_to_fit(); // free up memory ahead of forks
}

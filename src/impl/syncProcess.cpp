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

#include "syncProcess.hpp"
#include "syncer.hpp"
#include "file.hpp"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <csignal>
#include <boost/tokenizer.hpp>

extern "C" {
	#include <unistd.h>
	#include <fcntl.h>
}

SyncProcess::SyncProcess(Syncer *parent, int id, int nproc, std::vector<File> &queue)
	: 	id_(id),
		inc_(nproc),
		pid_(0),
		max_mem_usage_(parent->max_mem_usage_),
		start_mem_usage_(parent->start_mem_usage_),
		curr_payload_bytes_(0),
		destination_(parent->destination_),
		sending_to_(*destination_),
		file_itr_(queue.begin()),
		payload_(parent->start_payload_){
	
	curr_mem_usage_ = start_mem_usage_;
	
	start_payload_sz_ = payload_.size();
	
	std::advance(file_itr_, id_);
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
	return curr_payload_bytes_;
}

uintmax_t SyncProcess::payload_count(void) const{
	return payload_.size() - start_payload_sz_ - 2; // subtract NULL and destination
}

void SyncProcess::add(const std::vector<File>::iterator &itr){
	payload_.push_back(itr->path());
	curr_mem_usage_ += itr->path_len() + 1 + sizeof(char *);
	curr_payload_bytes_ += itr->size();
}

bool SyncProcess::full_test(const File &file) const{
	return (curr_mem_usage_ + file.path_len() + 1 + sizeof(char *) >= max_mem_usage_);
}

void SyncProcess::consume(std::vector<File> &queue){
	while(file_itr_ < queue.end() && !full_test(*file_itr_)){
		add(file_itr_);
		std::advance(file_itr_, inc_);
	}
	if(!destination_->empty()){
		sending_to_ = *destination_;
		payload_.push_back((char *)destination_->c_str());
	}
	payload_.push_back(NULL);
}

void SyncProcess::sync_batch(){
	pid_ = fork(); // create child process
	int error;
	switch(pid_){
		case -1:
			error = errno;
			Logging::log.error(std::string("Forking failed: ") + strerror(error));
			l::exit(EXIT_FAILURE);
		case 0: // child process
			{
				signal(SIGINT, SIG_DFL);
				int null_fd = open("/dev/null", O_WRONLY);
				dup2(null_fd, 1);
				dup2(null_fd, 2);
				close(null_fd);
				execvp(payload_[0], payload_.data());
				error = errno;
				dump_argv(error);
				int execvp_errno = -error;
				exit(execvp_errno);
			}
			break;
		default: // parent process
			Logging::log.message(std::to_string(pid_) + " started.", 2);
			break;
	}
}

void SyncProcess::reset(void){
	payload_.resize(start_payload_sz_);
	curr_mem_usage_ = start_mem_usage_;
	curr_payload_bytes_ = 0;
}

bool SyncProcess::done(const std::vector<File> &queue) const{
	return (file_itr_ >= queue.end()); // no room to advance file_itr_ by nproc_
}

const std::string &SyncProcess::destination(void) const{
	return sending_to_;
}

void SyncProcess::dump_argv(int error) const{
	std::string log_location = "/var/log/cephgeorep";
	if(!fs::exists(log_location))
		fs::create_directories(log_location);
	std::stringstream log_path_ss;
	std::time_t now = std::time(nullptr);
	log_path_ss << log_location << "/exec_fail_" << std::put_time(std::localtime(&now), "%F_%T_%z") << ".log";
	std::string log_path = log_path_ss.str();
	if(fs::exists(log_path)){
		int i = 1;
		while(fs::exists(log_path + "." + std::to_string(i)))
			i++;
		log_path += "." + std::to_string(i);
	}
	std::ofstream f;
	f.open(log_path, std::ios::trunc);
	if(!f.is_open()){
		Logging::log.error("Could not dump argv to log file.");
		return;
	}
	std::string msg = (error > 0)? strerror(error) : Logging::log.rsync_error(-error);
	f << error << " : " << msg << std::endl;
	for(const char *arg : payload_){
		if(arg)
			f << arg << std::endl;
	}
	f.close();
}

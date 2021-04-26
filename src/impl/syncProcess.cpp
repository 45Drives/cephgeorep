#include "syncProcess.hpp"
#include "syncer.hpp"
#include "file.hpp"
#include <unistd.h>
#include <boost/tokenizer.hpp>

extern "C" {
	#include <unistd.h>
	#include <fcntl.h>
}

SyncProcess::SyncProcess(Syncer *parent, int id, int nproc, std::vector<File> &queue)
	: 	id_(id),
		inc_(nproc),
		max_mem_usage_(parent->max_mem_usage_),
		start_mem_usage_(parent->start_mem_usage_),
		curr_mem_usage_(start_mem_usage_),
		curr_payload_bytes_(0),
		exec_bin_(parent->exec_bin_),
		exec_flags_(parent->exec_flags_),
		destination_(parent->destination_),
		file_itr_(queue.begin()){ // start at offset of id and later inc by nproc
	payload_.push_back((char *)exec_bin_.c_str());
	
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
		payload_.push_back(flag);
	}
	
	start_payload_sz_ = payload_.size();
	
	std::advance(file_itr_, id_);
}

SyncProcess::~SyncProcess(){}

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
	itr->disown_path();
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
	sending_to_ = *destination_;
	if(!destination_->empty()) payload_.push_back((char *)destination_->c_str());
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
				int null_fd = open("/dev/null", O_WRONLY);
				dup2(null_fd, 1);
				dup2(null_fd, 2);
				close(null_fd);
				execvp(payload_[0], payload_.data());
				int execvp_errno = -errno;
				exit(execvp_errno);
			}
			break;
		default: // parent process
			Logging::log.message(std::to_string(pid_) + " started.", 2);
			break;
	}
}

void SyncProcess::reset(void){
	for(
		std::vector<char *>::iterator itr = std::next(payload_.begin(), start_payload_sz_);
		itr != std::prev(payload_.end(), 2); // 2 before end to avoid deleting dest and NULL
		++itr
	){
		delete[] *itr; // free memory taken by path
	}
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

#include "syncProcess.hpp"
#include "syncer.hpp"
#include <unistd.h>

SyncProcess::SyncProcess(Syncer *parent, uintmax_t max_bytes_sz, std::vector<File> &queue)
    : exec_bin_(parent->exec_bin_), exec_flags_(parent->exec_flags_), destination_(parent->destination_){
	id_ = -1;
	curr_bytes_sz_ = 0;
	max_arg_sz_ = parent->max_arg_sz_;
	start_arg_sz_= curr_arg_sz_ = parent->start_arg_sz_;
	max_bytes_sz_ = max_bytes_sz;
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

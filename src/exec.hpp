/*
    Copyright (C) 2019-2021 Joshua Boudreau
    
    This file is part of cephgeorep.

    cephgeorep is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    cephgeorep is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "alert.hpp"
#include <vector>
#include <list>
#include <thread>
#include <boost/filesystem.hpp>

#define SSH_FAIL 255
#define NOT_INSTALLED 127
#define SUCCESS 0
#define PERM_DENY 23

namespace fs = boost::filesystem;

class SyncProcess{
private:
  int id_;
  pid_t pid_;
  size_t start_arg_sz;
  size_t max_arg_sz;
  size_t curr_arg_sz;
  uintmax_t max_bytes_sz;
  uintmax_t curr_bytes_sz;
  std::vector<fs::path> files;
  std::vector<char *> garbage;
public:
  pid_t pid(){ return pid_; }
  SyncProcess(const size_t &max_arg_sz_, const size_t &start_arg_sz_, const uintmax_t &max_bytes_sz_){
    id_ = -1;
    curr_bytes_sz = 0;
    max_arg_sz = max_arg_sz_;
    start_arg_sz = curr_arg_sz = start_arg_sz_;
    max_bytes_sz = max_bytes_sz_;
  }
  ~SyncProcess(){
    if(id_ != -1) Log("Proc " + std::to_string(id_) + ": done.",1);
    for(char *i : garbage){
      delete [] i;
    } 
  }
  void set_id(int id){
    id_ = id;
  }
  uintmax_t payload_sz(void){
    return curr_bytes_sz;
  }
  void add(const fs::path &file){
    files.emplace_back(file);
    curr_arg_sz += strlen(file.c_str()) + 1 + sizeof(char *);
    curr_bytes_sz += fs::file_size(file);
  }
  bool full_test(const fs::path &file){
    return (curr_arg_sz + strlen(file.c_str()) + 1 + sizeof(char *) >= max_arg_sz)
        || (curr_bytes_sz + fs::file_size(file) > max_bytes_sz);
  }
  bool large_file(const fs::path &file){
    return curr_bytes_sz < max_bytes_sz && fs::file_size(file) >= max_bytes_sz;
  }
  void consume(std::list<fs::path> &queue){
    while(!queue.empty() && (!full_test(queue.front()) || large_file(queue.front()))){
      add(fs::path(queue.front()));
      queue.pop_front();
    }
  }
  void consume_one(std::list<fs::path> &queue){
    add(fs::path(queue.front()));
    queue.pop_front();
  }
  void sync_batch(void);
  // fork and execute sync program with file batch
  void clear_file_list(void){
    files.clear();
    curr_bytes_sz = 0;
    curr_arg_sz = start_arg_sz;
  }
};

void launch_procs(std::list<fs::path> &queue, int nproc, uintmax_t total_bytes);
// distribute queue amongst processes and launch processes

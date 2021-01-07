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

class Batch{
  friend class SyncProcess;
private:
  size_t max_sz;
  size_t curr_sz;
  std::vector<fs::path> files;
public:
  Batch(const size_t &max_sz_, const size_t &start_sz_) : files(){
    max_sz = max_sz_;
    curr_sz = start_sz_;
  }
  void add(const fs::path &file){
    files.emplace_back(file);
    curr_sz += strlen(file.c_str()) + 1 + sizeof(char *);
  }
  bool full_test(const fs::path &file){
    return (curr_sz + strlen(file.c_str()) + 1 + sizeof(char *) >= max_sz);
  }
  int size(void) const{
    return files.size();
  }
};

class SyncProcess{
private:
  int id_ = -1;
  pid_t pid_;
  size_t max_sz;
  size_t start_sz;
  std::list<Batch> batches;
  std::vector<char *> garbage;
public:
  pid_t pid(){ return pid_; }
  SyncProcess(const size_t &max_sz_, const size_t &start_sz_) : batches(1, {max_sz_, start_sz_}){
    max_sz = max_sz_;
    start_sz = start_sz_;
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
  void add(const fs::path &file){
    if(batches.back().full_test(file)){
      batches.emplace_back(max_sz, start_sz);
    }
    batches.back().add(file);
  }
  void pop_batch(void){
    batches.pop_front();
  }
  bool batches_done(void){
    return batches.empty();
  }
  void sync_batch(void);
  // fork and execute sync program with file batch
};

void launch_procs(std::list<fs::path> &queue, int nproc);
// distribute queue amongst processes and launch processes

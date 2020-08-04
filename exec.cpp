/*
    Copyright (C) 2019 Joshua Boudreau
    
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

//#define DEBUG_BATCHES

#include "exec.hpp"
#include "config.hpp"
#include "alert.hpp"
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>
#include <chrono>
#include <sys/resource.h>

#ifdef DEBUG_BATCHES
#include <iostream>
#endif

#define MAX_SZ_LIM 3*(8*1024*1024)/4

namespace fs = boost::filesystem;

void split_batches(std::list<fs::path> &queue){
#ifdef DEBUG_BATCHES
  int num_batches = 0;
  std::vector<int> batch_size;
#endif
  struct rlimit lims;
  getrlimit(RLIMIT_STACK, &lims);
  size_t arg_max_sz = lims.rlim_cur / 4;
  if(arg_max_sz > MAX_SZ_LIM) arg_max_sz = MAX_SZ_LIM;
  arg_max_sz -= 4*2048; // allow 2048 bytes of headroom
  while(!queue.empty()){
    std::list<fs::path> batch;
    size_t curr_sz = config.env_sz + config.execBin.length() + config.execFlags.length();
    curr_sz += sizeof(char *) * 2;
    if(config.sync_remote_dest[0] != '\0'){
      curr_sz += strlen(config.sync_remote_dest) + 1 + sizeof(char *);
    }
    curr_sz += sizeof(NULL);
    while(!queue.empty() && (curr_sz + strlen(queue.front().c_str()) + 1 + sizeof(char *)) < arg_max_sz){
      batch.emplace_back(queue.front());
      curr_sz += strlen(queue.front().c_str()) + 1 + sizeof(char *);
      queue.pop_front();
    }
#ifdef DEBUG_BATCHES
    batch_size.push_back(curr_sz);
    num_batches++;
    std::cout << "Batch: " << num_batches << std::endl;
    for(fs::path i : batch){
      std::cout << i << std::endl;
    }
  std::cout << "Max arg size: " << arg_max_sz << std::endl;
#else
    launch_syncBin(batch);
#endif
  }
#ifdef DEBUG_BATCHES
  std::cout << "Env size: " << config.env_sz << std::endl;
  std::cout << "Batches: " << num_batches << std::endl;
  for(int i : batch_size){
    std::cout << "size: " << i << std::endl;
  }
#endif
}

void launch_syncBin(std::list<fs::path> &queue){
  pid_t pid;
  int status;
  std::vector<char *> argv;
  std::vector<char *> garbage;
  argv.push_back((char *)config.execBin.c_str());
  
  std::string binStr(config.execBin);
  
  boost::tokenizer<boost::escaped_list_separator<char>> tokens(
    config.execFlags,
    boost::escaped_list_separator<char>(
      std::string(""), std::string(" "), std::string("\"\'")
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
    garbage.push_back(flag);
  }
  
  for(fs::path i : queue){
    char *path = new char[i.string().length()+1];
    std::strcpy(path,i.c_str());
    argv.push_back(path);
    garbage.push_back(path);
  }
  
  // push back host:/path/to/backup
  if(config.sync_remote_dest[0] != '\0') argv.push_back(config.sync_remote_dest);
  
  argv.push_back(NULL);
  
  do{
    Log("Launching " + binStr + " " + config.execFlags + " with " + std::to_string(queue.size()) + " files.", 1);
    
    pid = fork(); // create child process
    switch(pid){
    case -1:
      error(FORK);
      break;
    case 0: // child process
      pid = getpid();
      Log(binStr + " process created with PID " + std::to_string(pid), 2);
      execvp(argv[0],&argv[0]);
      error(LAUNCH_RSYNC);
      break;
    default: // parent process
      // wait for rsync to finish before removing snapshot
      if((pid = wait(&status)) < 0)
        error(WAIT_RSYNC);
      Log(binStr + " process exited with status " + std::to_string(WEXITSTATUS(status)),2);
      break;
    }
    switch(WEXITSTATUS(status)){
    case SUCCESS:
      Log("Done.",1);
      break;
    case SSH_FAIL:
      Log(binStr + " failed to connect to remote backup server.\n"
      "Is the server running and connected to your network?\n"
      "Trying again in 25 seconds.",0);
      std::this_thread::sleep_for(std::chrono::seconds(25));
      break;
    case NOT_INSTALLED:
      error(NO_RSYNC);
      break;
    case PERM_DENY:
      error(NO_PERM);
      break;
    default:
      error(UNK_RSYNC_ERR);
      break;
    }
  }while(WEXITSTATUS(status) == SSH_FAIL);
  
  // garbage cleanup
  for(auto i : garbage){
    delete i;
  }
}

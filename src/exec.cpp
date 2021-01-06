/*
    Copyright (C) 2019-2020 Joshua Boudreau
    
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
#include <list>
#include <algorithm>
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

inline size_t get_max_arg_sz(){
  struct rlimit lims;
  getrlimit(RLIMIT_STACK, &lims);
  size_t arg_max_sz = lims.rlim_cur / 4;
  if(arg_max_sz > MAX_SZ_LIM) arg_max_sz = MAX_SZ_LIM;
  arg_max_sz -= 4*2048; // allow 2048 bytes of headroom
  return arg_max_sz;
}

inline size_t get_start_arg_sz(){
  size_t size = config.env_sz // size of ENV
              + config.execBin.length() // length of executable name
              + config.execFlags.length() // length of flags
              + sizeof(char *) * 2 // size of char pointers
              + sizeof(NULL);
  if(config.sync_remote_dest[0] != '\0'){
    size += strlen(config.sync_remote_dest) + 1 + sizeof(char *);
  }
  return size;
}

// void launch_procs(std::list<fs::path> &queue){
//   size_t arg_max_sz = get_max_arg_sz();
//   int nproc = config.rsync_nproc;
//   
//   if(nproc > 1){
//     int files_per_proc = queue.size() / nproc + (queue.size() % nproc != 0);
//     std::vector<proc_t *> procs;
//     std::list<fs::path>::iterator itr = queue.begin();
//     for(int i = 0; i < nproc && itr != queue.end(); i++){
//       int count = 0;
//       proc_t *ptr = new proc_t;
//       while(itr != queue.end() && count < files_per_proc){
//         ptr->queue.emplace_back(*itr);
//         count++;
//         itr++;
//       }
//       procs.push_back(ptr);
//     }
//     for(proc_t *proc : procs){
//       proc->tid = std::thread(split_batches, std::ref(proc->queue), arg_max_sz);
//     }
//     for(proc_t *proc : procs){
//       proc->tid.join();
//       delete proc;
//     }
//   }else{
//     split_batches(queue, arg_max_sz);
//   }
// }

inline bool procs_done(std::list<SyncProcess> &procs){
  for(std::list<SyncProcess>::iterator itr = procs.begin(); itr != procs.end(); ++itr){
    if(!itr->batches_done()) return false;
  }
  return true;
}

void split_batches(std::list<fs::path> &queue, int nproc){
  int wstatus;
  size_t max_sz = get_max_arg_sz();
  size_t start_sz = get_start_arg_sz();
  std::list<SyncProcess> procs(nproc, {max_sz, start_sz});
  std::list<SyncProcess>::iterator proc_itr = procs.begin();
  // distribute files amongst processes
  while(!queue.empty()){
    proc_itr->add(queue.front());
    queue.pop_front();
    if(++proc_itr == procs.end()) proc_itr = procs.begin();
  }
  // start each process
  for(proc_itr = procs.begin(); proc_itr != procs.end(); ++proc_itr){
    proc_itr->sync_batch();
  }
  while(!procs_done(procs)){ // while files are remaining in batch queues
    // wait for a child to change state then relaunch remaining batches
    pid_t exited_pid = wait(&wstatus);
    // find which object PID belongs to
    std::list<SyncProcess>::iterator exited_proc = std::find_if (
      procs.begin(), procs.end(),
      [&exited_pid](SyncProcess &proc){
        return proc.pid() == exited_pid;
      }
    );
    // check exit code
    switch(WEXITSTATUS(wstatus)){
    case SUCCESS:
      Log(std::to_string(exited_pid) + " done.",1);
      exited_proc->pop_batch(); // remove synced batch
      break;
    case SSH_FAIL:
      Log(config.execBin + " failed to connect to remote backup server.\n"
      "Is the server running and connected to your network?\n",0);
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
    if(exited_proc->batches_done()){
      procs.erase(exited_proc);
    }else{
      exited_proc->sync_batch();
    }
  }
}

void SyncProcess::sync_batch(){
  std::vector<char *> argv;
  
  argv.push_back((char *)config.execBin.c_str());
  
  // push back flags
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
  
  // for each file of batch
  for(std::vector<fs::path>::iterator fitr = batches.front().files.begin(); fitr != batches.front().files.end(); ++fitr){
    char *path = new char[fitr->string().length()+1];
    std::strcpy(path,fitr->c_str());
    argv.push_back(path);
    garbage.push_back(path); // for cleanup
  }
  
//     batches.pop_front(); // remove after ensuring rsync successful
  
  if(config.sync_remote_dest[0] != '\0') argv.push_back(config.sync_remote_dest);
  argv.push_back(NULL);
  
  Log("Launching " + config.execBin + " " + config.execFlags + " with " + std::to_string(batches.front().size()) + " files.", 1);
  
  pid_ = fork(); // create child process
  switch(pid_){
  case -1:
    error(FORK);
    break;
  case 0: // child process
    execvp(argv[0],&argv[0]);
    error(LAUNCH_RSYNC);
    break;
  default: // parent process
    Log(config.execBin + " process created with PID " + std::to_string(pid_), 2);
    break;
  }
}

// void launch_syncBin(std::list<fs::path> &queue){
//   pid_t pid;
//   int status;
//   std::vector<char *> argv;
//   std::vector<char *> garbage;
//   argv.push_back((char *)config.execBin.c_str());
//   
//   std::string binStr(config.execBin);
//   
//   boost::tokenizer<boost::escaped_list_separator<char>> tokens(
//     config.execFlags,
//     boost::escaped_list_separator<char>(
//       std::string(""), std::string(" "), std::string("\"\'")
//     )
//   );
//   for(
//     boost::tokenizer<boost::escaped_list_separator<char>>::iterator itr = tokens.begin();
//     itr != tokens.end();
//     ++itr
//   ){
//     char *flag = new char[(*itr).length()+1];
//     strcpy(flag,(*itr).c_str());
//     argv.push_back(flag);
//     garbage.push_back(flag);
//   }
//   
//   for(fs::path i : queue){
//     char *path = new char[i.string().length()+1];
//     std::strcpy(path,i.c_str());
//     argv.push_back(path);
//     garbage.push_back(path);
//   }
//   
//   // push back host:/path/to/backup
//   if(config.sync_remote_dest[0] != '\0') argv.push_back(config.sync_remote_dest);
//   
//   argv.push_back(NULL);
//   
//   do{
//     Log("Launching " + binStr + " " + config.execFlags + " with " + std::to_string(queue.size()) + " files.", 1);
//     
//     pid = fork(); // create child process
//     switch(pid){
//     case -1:
//       error(FORK);
//       break;
//     case 0: // child process
//       pid = getpid();
//       Log(binStr + " process created with PID " + std::to_string(pid), 2);
//       execvp(argv[0],&argv[0]);
//       error(LAUNCH_RSYNC);
//       break;
//     default: // parent process
//       // wait for rsync to finish before removing snapshot
//       if((pid = wait(&status)) < 0)
//         error(WAIT_RSYNC);
//       Log(binStr + " process exited with status " + std::to_string(WEXITSTATUS(status)),2);
//       break;
//     }
//     switch(WEXITSTATUS(status)){
//     case SUCCESS:
//       Log("Done.",1);
//       break;
//     case SSH_FAIL:
//       Log(binStr + " failed to connect to remote backup server.\n"
//       "Is the server running and connected to your network?\n"
//       "Trying again in 25 seconds.",0);
//       std::this_thread::sleep_for(std::chrono::seconds(25));
//       break;
//     case NOT_INSTALLED:
//       error(NO_RSYNC);
//       break;
//     case PERM_DENY:
//       error(NO_PERM);
//       break;
//     default:
//       error(UNK_RSYNC_ERR);
//       break;
//     }
//   }while(WEXITSTATUS(status) == SSH_FAIL);
//   
//   // garbage cleanup
//   for(auto i : garbage){
//     delete i;
//   }
// }

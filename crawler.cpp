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

#include "crawler.hpp"
#include "alert.hpp"
#include "rctime.hpp"
#include "config.hpp"
#include "exec.hpp"
#include <boost/filesystem.hpp>
#include <sys/xattr.h>
#include <thread>
#include <chrono>
#include <signal.h>

namespace fs = boost::filesystem;

bool running = true;

void sigint_hdlr(int signum){
  // cleanup from termination
  writeLast_rctime(last_rctime);
  exit(signum);
}

void initDaemon(void){
  loadConfig();
  Log("Starting Ceph Georep Daemon.",1);
  if(config.log_level >= 2) dumpConfig();
  // enable signal handlers to save last_rctime
  signal(SIGINT, sigint_hdlr);
  signal(SIGTERM, sigint_hdlr);
  signal(SIGQUIT, sigint_hdlr);
  
  last_rctime = loadLast_rctime();
  
  if(!exists(config.sender_dir)) error(SND_DIR_DNE);
}

void pollBase(fs::path path){
  timespec rctime;
  std::list<fs::path> sync_queue;
  Log("Watching: " + path.string(),1);
  
  while(running){
    auto start = std::chrono::system_clock::now();
    if(checkForChange(path, last_rctime, rctime)){
      Log("Change detected in " + path.string(), 2);
      // create snapshot
      fs::path snapPath = takesnap(rctime);
      // wait for rctime to trickle to root
      std::this_thread::sleep_for(std::chrono::milliseconds(config.prop_delay_ms));
      // launch crawler in snapshot
      Log("Launching crawler",2);
      crawler(snapPath, sync_queue, snapPath); // enqueue if rctime > last_rctime
      // log list of new files
      if(config.log_level >= 2){ // skip loop if not logging
        Log("Files to sync:",2);
        for(auto i : sync_queue){
          Log(i.string(),2);
        }
      }
      Log("New files to sync: "+std::to_string(sync_queue.size())+".",2);
      // launch rsync
      if(!sync_queue.empty()) split_batches(sync_queue);
      // clear sync queue
      sync_queue.clear();
      // delete snapshot
      deletesnap(snapPath);
      // overwrite last_rctime
      last_rctime = rctime;
    }
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = end-start;
    if((int)elapsed.count() < config.sync_frequency) // if it took longer than sync freq, don't wait
      std::this_thread::sleep_for(std::chrono::seconds(config.sync_frequency - (int)elapsed.count()));
  }
}

void crawler(fs::path path, std::list<fs::path> &queue, const fs::path &snapdir){
  for(fs::directory_iterator itr{path};
  itr != fs::directory_iterator{}; *itr++){
    if((config.ignore_hidden == true &&
    (*itr).path().filename().string().front() == '.') ||
    (config.ignore_win_lock  == true &&
    (*itr).path().filename().string().substr(0,2) == "~$") ||
    (get_rctime(*itr) < last_rctime))
      continue;
    if(is_directory(*itr)){
      crawler(path/((*itr).path().filename()), queue, snapdir); // recurse
    }else{
      // cut path at sync dir for rsync /sync_dir/.snap/snapshotX/./rel_path/file
      queue.emplace_back(snapdir/fs::path(".")/fs::relative((*itr).path(),snapdir));
    }
  }
}

bool checkForChange(const fs::path &path, const timespec &last_rctime, timespec &rctime){
  bool change = false;
  timespec temp_rctime;
  for(fs::directory_iterator itr(path);
  itr != fs::directory_iterator(); *itr++){
    if((temp_rctime = get_rctime(*itr)) > last_rctime){
      change = true;
      if(temp_rctime > rctime) rctime = temp_rctime; // get highest
    }
  }
  return change;
}

fs::path takesnap(const timespec &rctime){
  std::string pid = std::to_string(getpid());
  fs::path snapPath = fs::path(config.sender_dir).append(".snap/"+pid+"snapshot"+rctime);
  Log("Creating snapshot: " + snapPath.string(), 2);
  fs::create_directories(snapPath, ec);
  if(ec) error(PATH_CREATE, ec);
  return snapPath;
}

bool deletesnap(fs::path path){
  Log("Removing snapshot: " + path.string(), 2);
  bool result = fs::remove(path, ec);
  if(ec) error(REMOVE_SNAP, ec);
  return result;
}

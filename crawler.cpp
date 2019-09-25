#include "crawler.hpp"
#include "alert.hpp"
#include "rctime.hpp"
#include "config.hpp"
#include "rsync.hpp"
#include <boost/filesystem.hpp>
#include <sys/xattr.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <vector>
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
  std::vector<fs::path> sync_queue;
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
      Log("Files to sync:",2);
      for(auto i : sync_queue){
        Log(i.string(),2);
      }
      Log("New files to sync: "+std::to_string(sync_queue.size())+".",2);
      // launch rsync
      if(!sync_queue.empty()) launch_rsync(sync_queue);
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

void crawler(fs::path path, std::vector<fs::path> &queue, const fs::path &snapdir){
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
      queue.push_back(snapdir/fs::path(".")/fs::relative((*itr).path(),snapdir));
    }
  }
}

bool checkForChange(const fs::path &path, const timespec &last_rctime, timespec &rctime){
  bool change = false;
  std::vector<timespec> rctimes;
  for(fs::directory_iterator itr(path);
  itr != fs::directory_iterator(); *itr++){
    if((rctime = get_rctime(*itr)) > last_rctime){
      change = true;
      rctimes.push_back(rctime);
    }
  }
  if(change){
    for(timespec i : rctimes)
      if(i > rctime) rctime = i;
    return true;
  }
  return false;
}

int count(fs::path path, FilesOrDirs choice){
  char buffer[XATTR_SIZE];
  int buffLen;
  int result = 0;
  switch(choice){
  case BOTH:
  case FILES:
    if((buffLen = getxattr(path.c_str(), "ceph.dir.files",
    buffer, XATTR_SIZE)) == ERR)
      error(READ_FILES_DIRS);
    result = stoi(std::string(buffer).substr(0, buffLen));
    if(choice == FILES) break; // else fall through
  case DIRS:
    if((buffLen = getxattr(path.c_str(), "ceph.dir.subdirs",
    buffer, XATTR_SIZE)) == ERR)
      error(READ_FILES_DIRS);
    result += stoi(std::string(buffer).substr(0, buffLen));
    break;
  default:
    break;
  }
  return result;
}

void read_directory(fs::path path,
std::vector<fs::path> &q, FilesOrDirs choice){
  copy_if(fs::directory_iterator(path),
  fs::directory_iterator(), back_inserter(q),
  [choice](fs::path p){ // lambda fn for conditional copy
    if(config.ignore_hidden == true && p.filename().string().front() == '.')
      return false; // early return for ignoring hidden files/dirs
    switch(choice){
    case FILES:
      return !is_directory(p);
    case DIRS:
      return is_directory(p);
    case BOTH:
    default:
      return true;
    }
  });
}

fs::path takesnap(const timespec &rctime){
  fs::path snapPath = fs::path(config.sender_dir).append(".snap/snapshot"+rctime);
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

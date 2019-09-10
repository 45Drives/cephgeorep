#include "crawler.hpp"
#include "alert.hpp"
#include "rctime.hpp"
#include "config.hpp"
#include <boost/filesystem.hpp>
#include <sys/xattr.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <vector>

bool running = true;

void pollBase(boost::filesystem::path path){
  last_rctime = loadLast_rctime();
  timespec rctime;
  std::vector<boost::filesystem::path> sync_queue;
  
  while(running){
    if(checkForChange(path, last_rctime, rctime)){
      Log("Launching crawler",2);
      // create snapshot
      boost::filesystem::path snapPath = takesnap(rctime);
      // launch crawler in snapshot
      crawler(snapPath, sync_queue); // enqueue if rctime > last_rctime
      // log list of new files
      Log("Files to sync:",2);
      for(auto i : sync_queue){
        Log(i.string(),2);
      }
      Log("New files synced: "+std::to_string(sync_queue.size())+".",2);
      // launch rsync
      
      // clear sync queue
      sync_queue.clear();
      // delete snapshot
      deletesnap(snapPath);
      // overwrite last_rctime
      last_rctime = rctime;
    }
    std::this_thread::sleep_for(std::chrono::seconds(config.sync_frequency));
  }
  writeLast_rctime(last_rctime);
}

void crawler(boost::filesystem::path path, std::vector<boost::filesystem::path> &queue){
  //std::vector<boost::filesystem::path> dir_contents;
  
  // copy new files into queue
  copy_if(boost::filesystem::directory_iterator(path),
  boost::filesystem::directory_iterator(), back_inserter(queue),
  [](boost::filesystem::path p){ // lambda fn for conditional copy
    if(config.ignore_hidden == true && p.filename().string().front() == '.')
      return false; // early return for ignoring hidden files/dirs
    if(!is_directory(p) && get_rctime(p) > last_rctime){
      return true; // enque files with mtime > last_rctime
    }
    return false;
  });
  
  // conditionally recurse into each subdirectory
  for(boost::filesystem::directory_iterator itr{path};
  itr != boost::filesystem::directory_iterator{}; *itr++){
    if(is_directory(*itr) && get_rctime(*itr) > last_rctime)
      crawler(path/((*itr).path().filename()), queue);
  }
}

bool checkForChange(const boost::filesystem::path &path, const timespec &last_rctime, timespec &rctime){
  bool change = false;
  std::vector<timespec> rctimes;
  for(boost::filesystem::directory_iterator itr{path};
  itr != boost::filesystem::directory_iterator{}; *itr++){
    rctime = get_rctime(*itr);
    if(rctime > last_rctime){
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

int count(boost::filesystem::path path, FilesOrDirs choice){
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
    if(choice == FILES) break;
  case DIRS:
    if((buffLen = getxattr(path.c_str(), "ceph.dir.subdirs",
    buffer, XATTR_SIZE)) == ERR)
      error(READ_FILES_DIRS);
    result += stoi(std::string(buffer).substr(0, buffLen));
    break;
  }
  return result;
}

void read_directory(boost::filesystem::path path,
std::vector<boost::filesystem::path> &q, FilesOrDirs choice){
  copy_if(boost::filesystem::directory_iterator(path),
  boost::filesystem::directory_iterator(), back_inserter(q),
  [choice](boost::filesystem::path p){ // lambda fn for conditional copy
    if(config.ignore_hidden == true && p.filename().string().front() == '.')
      return false; // early return for ignoring hidden files/dirs
    switch(choice){
    case FILES:
      return !is_directory(p);
    case DIRS:
      return is_directory(p);
    default:
      return true;
    }
  });
}

boost::filesystem::path takesnap(const timespec &rctime){
  boost::filesystem::path snapPath = boost::filesystem::path(config.sender_dir).append(".snap/snapshot"+rctime);
  Log("Creating snapshot: " + snapPath.string(), 2);
  boost::filesystem::create_directories(snapPath, ec);
  if(ec) error(PATH_CREATE, ec);
  return snapPath;
}

bool deletesnap(boost::filesystem::path path){
  Log("Removing snapshot: " + path.string(), 2);
  bool result = boost::filesystem::remove(path, ec);
  if(ec) error(REMOVE_SNAP, ec);
  return result;
}

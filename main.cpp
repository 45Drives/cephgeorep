/*
 * Dependancies: boost filesystem, rsync
 * 
 */

#include <iostream>
#include <string>
#include "config.hpp"
#include "rctime.hpp"
#include "crawler.hpp"
#include "alert.hpp"

int main(int argc, char *argv[]){
  loadConfig();
  if(config.log_level >= 2) dumpConfig();
  pollBase(config.sender_dir);
  
  /*
  dumpConfig();
  
  timespec rctime = get_rctime(config.sender_dir);
  Log("Last rctime: " + last_rctime, 2);
  Log("root rctime: " + rctime, 2);
  
  boost::filesystem::path snapPath = takesnap(rctime);
  
  std::vector<boost::filesystem::path> queue;
  
  read_directory(snapPath, queue, FILES);
  for(boost::filesystem::path i : queue){
    std::cout << i << std::endl;
  }
  std::cout << "Files: " << count(snapPath,FILES) << std::endl;
  queue.clear();
  read_directory(snapPath ,queue, DIRS);
  for(boost::filesystem::path i : queue){
    std::cout << i << std::endl;
  }
  std::cout << "Subdirs: " << count(snapPath ,DIRS) << std::endl;
  queue.clear();
  read_directory(snapPath ,queue, BOTH);
  for(boost::filesystem::path i : queue){
    std::cout << i << std::endl;
  }
  std::cout << "Both: " << count(snapPath ,BOTH) << std::endl;
  queue.clear();
  
  if(!deletesnap(snapPath))
    error(REMOVE_SNAP);
  
  writeLast_rctime(rctime);
  */
  
  return 0;
}

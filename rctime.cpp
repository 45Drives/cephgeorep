#include "rctime.hpp"
#include "alert.hpp"
#include "config.hpp"
#include <ctime>
#include <string>
#include <fstream>
#include <boost/filesystem.hpp>
#include <sys/xattr.h>
#include <sys/stat.h>

timespec last_rctime;

timespec loadLast_rctime(void){
  Log("Reading last rctime from disk.",3);
  timespec rctime;
  std::ifstream f(config.last_rctime.c_str());
  std::string str;
  if(!f){
    init_last_rctime();
    f.open(config.last_rctime.c_str());
  }
  getline(f, str, '.');
  rctime.tv_sec = (time_t)stoul(str);
  getline(f, str);
  rctime.tv_nsec = stol(str);
  return rctime;
}

void writeLast_rctime(const timespec &rctime){
  std::ofstream f(config.last_rctime.c_str());
  if(!f){
    init_last_rctime();
    f.open(config.last_rctime.c_str());
  }
  f << rctime.tv_sec << '.' << rctime.tv_nsec << std::endl;
  f.close();
}

void init_last_rctime(void){
  Log(config.last_rctime.string() + " does not exist. Creating and initializing"
  "to 0.0.",2);
  boost::filesystem::create_directories(config.last_rctime.parent_path());
  std::ofstream f(config.last_rctime.c_str());
  f << "0.0" << std::endl;
  f.close();
}

timespec get_rctime(const boost::filesystem::path &path){
  timespec rctime;
  switch(is_directory(path)){
  case true: // dir
    {
    char value[XATTR_SIZE];
    if(getxattr(path.c_str(), "ceph.dir.rctime", value, XATTR_SIZE) == ERR)
      error(READ_RCTIME);
    std::string str(value);
    rctime.tv_sec = (time_t)stoul(str.substr(0,str.find('.')));
    // value = <seconds> + '.09' + <nanoseconds>
    // +3 is to remove the '.09'
    rctime.tv_nsec = stol(str.substr(str.find('.')+3, std::string::npos));
    break;
    }
  case false: // file
    {
    struct stat t_stat;
    if(stat(path.c_str(), &t_stat) == ERR)
      error(READ_MTIME);
    rctime.tv_sec = t_stat.st_mtim.tv_sec;
    rctime.tv_nsec = t_stat.st_mtim.tv_nsec;
    break;
    }
  }
  return rctime;
}

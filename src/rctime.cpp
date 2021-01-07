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

#include "rctime.hpp"
#include "alert.hpp"
#include "config.hpp"
#include <boost/filesystem.hpp>
#include <sys/xattr.h>
#include <sys/stat.h>

namespace fs = boost::filesystem;

timespec last_rctime = {0};

timespec loadLast_rctime(void){
  Log("Reading last rctime from disk.",2);
  timespec rctime;
  std::ifstream f(config.last_rctime.c_str());
  std::string str;
  if(!f){
    init_last_rctime();
    f.open(config.last_rctime.c_str());
  }
  try{
    getline(f, str, '.');
    rctime.tv_sec = (time_t)stoul(str);
    getline(f, str);
    rctime.tv_nsec = stol(str);
  }catch(std::invalid_argument){
    // last_rctime.dat corrupted
    Log(config.last_rctime.string()+" is corrupt. Reinitializing.",0);
    init_last_rctime();
    rctime.tv_sec = rctime.tv_nsec = 0;
  }
  return rctime;
}

void writeLast_rctime(const timespec &rctime){
  Log("Writing last rctime to disk.",2);
  std::ofstream f(config.last_rctime.c_str());
  if(!f){
    init_last_rctime();
    f.open(config.last_rctime.c_str());
  }
  f << rctime << std::endl;
  f.close();
}

void init_last_rctime(void){
  Log(config.last_rctime.string() + " does not exist. Creating and initializing to 0.0.",2);
  fs::create_directories(config.last_rctime.parent_path(), ec);
  if(ec) error(PATH_CREATE, ec);
  std::ofstream f(config.last_rctime.c_str());
  f << "0.0" << std::endl;
  f.close();
}

timespec get_rctime(const fs::path &path){
  timespec rctime;
  switch(is_directory(path)){
  case true: // dir
    {
    char value[XATTR_SIZE];
    if(getxattr(path.c_str(), "ceph.dir.rctime", value, XATTR_SIZE) == ERR){
      warning(READ_RCTIME);
      Log("Ignoring " + path.string(),0);
      rctime.tv_sec = 0;
      rctime.tv_nsec = 0;
    }else{
      std::string str(value);
      rctime.tv_sec = (time_t)stoul(str.substr(0,str.find('.')));
      // value = <seconds> + '.09' + <nanoseconds>
      // +3 is to remove the '.09'
      rctime.tv_nsec = stol(str.substr(str.find('.')+3, std::string::npos));
    }
    break;
    }
  case false: // file
    {
    struct stat t_stat;
    if(lstat(path.c_str(), &t_stat) == ERR){
      warning(READ_MTIME);
      Log("Ignoring " + path.string(),0);
      rctime.tv_sec = 0;
      rctime.tv_nsec = 0;
    }else{
      rctime.tv_sec = t_stat.st_mtim.tv_sec;
      rctime.tv_nsec = t_stat.st_mtim.tv_nsec;
    }
    break;
    }
  }
  return rctime;
}

std::string &operator+(std::string lhs, timespec rhs){
  return lhs.append(std::to_string(rhs.tv_sec) + "." + std::to_string(rhs.tv_nsec));
}

std::string &operator+(timespec lhs, std::string rhs){
  return (std::to_string(lhs.tv_sec) + "." + std::to_string(lhs.tv_nsec)).append(rhs);
}

bool operator>(const timespec &lhs, const timespec &rhs){
  return (lhs.tv_sec > rhs.tv_sec) ||
  ((lhs.tv_sec == rhs.tv_sec) && (lhs.tv_nsec > rhs.tv_nsec));
}

bool operator==(const timespec &lhs, const timespec &rhs){
  return (lhs.tv_sec == rhs.tv_sec) && (lhs.tv_nsec == rhs.tv_nsec);
}

bool operator>=(const timespec &lhs, const timespec &rhs){
  return (lhs > rhs) || (lhs == rhs);
}

bool operator<(const timespec &lhs, const timespec &rhs){
  return !(lhs >= rhs);
}

bool operator<=(const timespec &lhs, const timespec &rhs){
  return !(lhs > rhs);
}

std::ostream &operator<<(std::ostream &stream, const struct timespec &rhs){
  stream << rhs.tv_sec << "." << rhs.tv_nsec;
  return stream;
}

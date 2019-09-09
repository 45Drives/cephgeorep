#include "alert.hpp"
#include "config.hpp"
#include <iostream>
#include <string>

std::string errors[NUM_ERRS] = {
  "Error opening config file.",
  "Error creating path.",
  "Error reading ceph.dir.rctime attribute of directory. Are you using CEPH?",
  "Error reading mtime attribute of file."
};

void error(int err){
  std::cerr << errors[err] << std::endl;
  exit(1);
}

void Log(std::string msg, int lvl){
  if(config.log_level >= lvl) std::cout << msg << std::endl;
}

std::string &operator+(std::string lhs, timespec rhs){
  return lhs.append(std::to_string(rhs.tv_sec) + "." + std::to_string(rhs.tv_nsec));
}

#include "alert.hpp"
#include "config.hpp"
#include <iostream>
#include <string>
#include <boost/system/error_code.hpp>

boost::system::error_code ec;

std::string errors[NUM_ERRS] = {
  "Error opening config file.",
  "Error creating path.",
  "Error reading ceph.dir.rctime attribute of directory. Are you using CEPH?",
  "Error reading mtime attribute of file.",
  "Error reading number of files or directories in subdirectory.",
  "Error removing snapshot directory.",
  "Error forking process.",
  "Error launching rsync.",
  "Error while waiting for rsync to exit."
};

void error(int err, boost::system::error_code ec_){
  std::cerr << errors[err] << std::endl;
  exit(ec_.value());
}

void Log(std::string msg, int lvl){
  if(config.log_level >= lvl) std::cout << msg << std::endl;
}

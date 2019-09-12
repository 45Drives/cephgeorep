#pragma once

#include <string>
#include <boost/system/error_code.hpp>

extern boost::system::error_code ec; // error code for system fucntions

#define NUM_ERRS 11

enum {
  OPEN_CONFIG, PATH_CREATE, READ_RCTIME, READ_MTIME, READ_FILES_DIRS,
  REMOVE_SNAP, FORK, LAUNCH_RSYNC, WAIT_RSYNC, NO_RSYNC, UNK_RSYNC_ERR
};

void error(int err, boost::system::error_code ec_ = {1,boost::system::generic_category()});
// print error to std::cerr and exit with error code 1

void Log(std::string msg, int lvl);
// print msg to std::cout given config.log_level >= lvl

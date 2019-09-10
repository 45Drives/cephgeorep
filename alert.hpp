#pragma once

#include <string>
#include <boost/system/error_code.hpp>

extern boost::system::error_code ec; // error code for system fucntions

#define NUM_ERRS 6

enum {
  OPEN_CONFIG, PATH_CREATE, READ_RCTIME, READ_MTIME, READ_FILES_DIRS,
  REMOVE_SNAP
};

void error(int err, boost::system::error_code ec_ = {1,boost::system::generic_category()});
// print error to std::cerr and exit with error code 1

void Log(std::string msg, int lvl);
// print msg to std::cout given config.log_level >= lvl

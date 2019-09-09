#pragma once

#include <string>

#define NUM_ERRS 4

enum {OPEN_CONFIG, PATH_CREATE, READ_RCTIME, READ_MTIME};

void error(int err);
// print error to std::cerr and exit with error code 1

void Log(std::string msg, int lvl);
// print msg to std::cout given config.log_level >= lvl

std::string &operator+(std::string lhs, timespec rhs);
// for printing log messages with timespecs

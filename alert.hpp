#pragma once

#include <string>

#define NUM_ERRS 2

enum {OPEN_CONFIG, PATH_CREATE};

void error(int err);

void Log(std::string msg, int lvl);

#include "alert.hpp"
#include "config.hpp"
#include <iostream>
#include <string>

std::string errors[NUM_ERRS] = {
  "Error opening config file.",
  "Error creating path."
};

void error(int err){
  std::cerr << errors[err] << std::endl;
  exit(1);
}

void Log(std::string msg, int lvl){
  if(config.log_level >= lvl) std::cout << msg << std::endl;
}

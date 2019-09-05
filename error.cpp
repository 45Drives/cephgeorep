#include "error.hpp"
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

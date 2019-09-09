/*
 * Dependancies: boost filesystem, rsync
 * 
 */

#include <iostream>
#include <string>
#include "config.hpp"
#include "rctime.hpp"
#include "crawler.hpp"
#include "alert.hpp"

int main(int argc, char *argv[]){
  loadConfig();
  last_rctime = loadLast_rctime();
  std::cout << config.log_level << std::endl;
  Log("Last rctime: ", 1);
  return 0;
}

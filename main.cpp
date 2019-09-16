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
  initDaemon();
  pollBase(config.sender_dir);
  return 0;
}

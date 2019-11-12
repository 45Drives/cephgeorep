/*
 * Dependancies: boost filesystem, rsync
 * 
 */
 
#include <iostream>

#include "config.hpp"
#include "rctime.hpp"
#include "crawler.hpp"
#include "alert.hpp"

int main(int argc, char *argv[]){
  if(argc > 1){
    if(argc > 2 && (strcmp(argv[1],"-c") == 0 || strcmp(argv[1],"--config") == 0)){
      config_path = std::string(argv[2]);
    }else{
      std::cerr << "Unknown argument: " << argv[1] << std::endl;
    }
  }
  initDaemon();
  pollBase(config.sender_dir);
  return 0;
}

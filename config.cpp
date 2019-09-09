#include "config.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>

Config config; // global config struct

void loadConfig(void){
  bool errors = false;
  std::string line, key, value;
  
  // open file
  boost::filesystem::path configPath(CONFIG_PATH);
  std::fstream configFile(configPath.c_str());
  if(!configFile) createConfig(configPath, configFile);
  
  // read file
  while(configFile){
    getline(configFile, key, '=');
    getline(configFile, value, '\n');
    if(key == "SND_SYNC_DIR"){
      config.sender_dir = value;
    }else if(key == "RECV_SYNC_HOST"){
      config.receiver_host = value;
    }else if(key == "RECV_SYNC_DIR"){
      config.receiver_dir = value;
    }else if(key == "LAST_RCTIME_DIR"){
      config.last_rctime = boost::filesystem::path(value).append(LAST_RCTIME_NAME);
    }else if(key == "SYNC_FREQ"){
      config.sync_frequency = stoi(value);
    }else if(key == "IGNORE_HIDDEN"){
      std::istringstream(value) >> std::boolalpha >> config.ignore_hidden >> std::noboolalpha;
    }else if(key == "RCTIME_PROP_DELAY"){
      config.prop_delay_ms = stoi(value);
    }else if(key == "COMPRESSION"){
      std::istringstream(value) >> std::boolalpha >> config.compress >> std::noboolalpha;
    }else if(key == "LOG_LEVEL"){
      config.log_level = stoi(value);
    }
  }
  
  // verify contents
  if(config.sender_dir.empty()){
    std::cerr << "Config does not contain a directory to search (SND_SYNC_DIR)\n";
    errors = true;
  }
  if(config.receiver_host.empty()){
    std::cerr << "Config does not contain a remote host address (RECV_SYNC_HOST)\n";
    errors = true;
  }
  if(config.receiver_dir.empty()){
    std::cerr << "Config does not contain a remote host directory (RECV_SYNC_DIR)\n";
    errors = true;
  }
  if(config.last_rctime.empty()){
    std::cerr << "Config does not contain a path to store last timestamp (LAST_RCTIME_DIR)\n";
    errors = true;
  }
  if(config.sync_frequency < 0){
    std::cerr << "Config sync frequency must be positive integer (SYNC_FREQ)\n";
    errors = true;
  }
  if(errors){
    std::cerr << "Please fix these mistakes in " << configPath << "." << std::endl;
    exit(1);
  }
}

void createConfig(const boost::filesystem::path &configPath, std::fstream &configFile){
  boost::filesystem::create_directories(configPath.parent_path());
  std::ofstream f(configPath.c_str());
  f <<
    "SND_SYNC_DIR=\n"
    "RECV_SYNC_HOST=\n"
    "RECV_SYNC_DIR=\n"
    "LAST_RCTIME_DIR=/var/lib/ceph/cephfssync/\n"
    "SYNC_FREQ=10\n"
    "IGNORE_HIDDEN=false\n"
    "RCTIME_PROP_DELAY=5000\n"
    "COMPRESSION=false\n"
    "LOG_LEVEL=1\n"
    "# 0 = minimum logging\n"
    "# 1 = basic logging\n"
    "# 2 = debug logging\n"
    "# sync frequency in seconds\n"
    "# propagation delay in milliseconds\n"
    "# Propagation delay is to account for the limit that Ceph can\n"
    "# propagate the modification time of a file all the way back to\n"
    "# the root of the sync directory.\n"
    "# Only use compression if your network connection to your\n"
    "# backup server is slow.\n";
  f.close();
  configFile.open(configPath.c_str()); // seek to beginning of file for input
}

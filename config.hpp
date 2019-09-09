#pragma once

#include <boost/filesystem.hpp>
#include <string>

#define CONFIG_PATH "/etc/ceph/cephfssyncd.conf"
#define LAST_RCTIME_NAME "last_rctime.dat"

struct Config{
  boost::filesystem::path sender_dir;
  std::string receiver_host;
  boost::filesystem::path receiver_dir;
  boost::filesystem::path last_rctime;
  int sync_frequency;
  bool ignore_hidden;
  int prop_delay_ms;
  bool compress;
  int log_level;
};

extern Config config;

void loadConfig(void);
// Load configuration parameters from file, create default config if none exists

void createConfig(const boost::filesystem::path &configPath, std::fstream &configFile);
// creates config directory and initializes config file. Returns config file
// stream in &configFile.

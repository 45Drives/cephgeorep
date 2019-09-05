#pragma once
#include <boost/filesystem.hpp>
#include <string>

#define CONFIG_PATH "/etc/ceph/cephfssyncd.conf"

struct Config{
  boost::filesystem::path sender_dir;
  std::string receiver_host;
  boost::filesystem::path receiver_dir;
  boost::filesystem::path last_rctime_dir;
  int sync_frequency;
  bool ignore_hidden;
  int prop_delay_ms;
  bool compress;
  int log_level;
};

void initDaemon(void);
// Initialize daemon - load config, retrieve last rctime from file

void loadConfig(void);
// Load configuration parameters from file, create default config if none exists

void createConfig(boost::filesystem::path configPath, std::fstream &configFile);
// creates config directory and initializes config file. Returns config file
// stream in &configFile.

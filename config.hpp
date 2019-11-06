#pragma once

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#define CONFIG_PATH "/etc/ceph/cephfssyncd.conf"
#define LAST_RCTIME_NAME "last_rctime.dat"

struct Config{
  int log_level;
  int sync_frequency;
  int prop_delay_ms;
  char *sync_remote_dest;
  bool ignore_hidden;
  bool ignore_win_lock;
  std::string remote_user;
  fs::path sender_dir;
  std::string receiver_host;
  fs::path receiver_dir;
  fs::path last_rctime;
  std::string execBin;
  std::string execFlags;
};

extern Config config;

void loadConfig(void);
// Load configuration parameters from file, create default config if none exists

void createConfig(const fs::path &configPath, std::fstream &configFile);
// creates config directory and initializes config file. Returns config file
// stream in &configFile.

void dumpConfig(void);
// dumps config contents to screen

/*
    Copyright (C) 2019-2020 Joshua Boudreau
    
    This file is part of cephgeorep.

    cephgeorep is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    cephgeorep is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#define DEFAULT_CONFIG_PATH "/etc/ceph/cephfssyncd.conf"
#define LAST_RCTIME_NAME "last_rctime.dat"

struct Config{
  int log_level = -1;
  int sync_frequency;
  int prop_delay_ms;
  size_t env_sz = 0;
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

extern std::string config_path;

extern Config config;

void loadConfig(void);
// Load configuration parameters from file, create default config if none exists

void strip_whitespace(std::string &str);

void verifyConfig(void);

void construct_rsync_remote_dest(void);

void createConfig(const fs::path &configPath, std::fstream &configFile);
// creates config directory and initializes config file. Returns config file
// stream in &configFile.

void dumpConfig(void);
// dumps config contents to screen

size_t find_env_size(char *envp[]);
// get environment size

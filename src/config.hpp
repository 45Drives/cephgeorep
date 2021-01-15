/*
 *    Copyright (C) 2019-2021 Joshua Boudreau
 *    
 *    This file is part of cephgeorep.
 * 
 *    cephgeorep is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License, or
 *    (at your option) any later version.
 * 
 *    cephgeorep is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License
 *    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <chrono>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#define DEFAULT_CONFIG_PATH "/etc/ceph/cephfssyncd.conf"
#define LAST_RCTIME_NAME "last_rctime.dat"

template<class T>
class ConfigOverride{
private:
	T value_;
	bool overridden_;
public:
	ConfigOverride(void){
		overridden_ = false;
	}
	ConfigOverride(const T &value_passed) : value_(value_passed){
		overridden_ = true;
	}
	~ConfigOverride(void) = default;
	const T &value(void) const{
		return value_;
	}
	const bool &overridden(void) const{
		return overridden_;
	}
};

struct ConfigOverrides{
	ConfigOverride<int> log_level_override;
	ConfigOverride<int> nproc_override;
};

class Crawler;

class Config{
	friend class Crawler;
	friend class Syncer;
private:
	// daemon settings
	int log_level_;
	int nproc_;
	bool ignore_hidden_;
	bool ignore_win_lock_;
	std::chrono::seconds sync_period_s_;
	std::chrono::milliseconds prop_delay_ms_;
	fs::path last_rctime_path_;
	std::string exec_bin_;
	std::string exec_flags_;
	
	// source
	fs::path base_path_;
	
	// target
	std::string remote_user_;
	std::string remote_host_;
	fs::path remote_directory_;
public:
	Config(const fs::path &config_path, const ConfigOverrides &config_overrides);
	~Config() = default;
	void init_config_file(const fs::path &config_path) const;
	void override_fields(const ConfigOverrides &config_overrides);
	void verify(const fs::path &config_path) const;
	void dump(void) const;
// 	void construct_destination(void);
};

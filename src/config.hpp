/*
 *    Copyright (C) 2019-2021 Joshua Boudreau <jboudreau@45drives.com>
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
/* ConfigOverride is used with command line flags
 * to allow for easy overridding of configuration values.
 */
class ConfigOverride{
private:
	T value_;
	/* The value to override with.
	 */
	bool overridden_;
	/* Whether or not the field is overridden.
	 * Since ConfigOverride objects are constructed in main()
	 * without a valuem, overridden_ will be set to false. If
	 * the value is updated through copying a newly constructed
	 * ConfigOverride object with a value, this will be true.
	 */
public:
	ConfigOverride(void) : value_() {
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
	/* Currently implemented configuration overrides.
	 */
	ConfigOverride<int> log_level_override;
	/* Overridden in main with -v --verbose and -q --quiet flags.
	 */
	ConfigOverride<int> nproc_override;
	/* Overridden in main with -n --nproc flag.
	 */
	ConfigOverride<int> threads_override;
	/* Overridden in main with -t --threads flag.
	 */
};

class Crawler;

class Config{
	friend class Crawler;
	friend class Syncer;
	/* allow Crawler and Syncer objects to access members.
	 */
private:
	// daemon settings
	int log_level_;
	/* Log level defined in config file for Logger class.
	 */
	int nproc_;
	/* Number of parallel sync processes.
	 */
	int threads_;
	/* Number of worker threads to search directory tree.
	 */
	bool ignore_hidden_;
	/* Ignore files starting with '.'.
	 */
	bool ignore_win_lock_;
	/* Ignore files starting with "~$"
	 */
	std::chrono::seconds sync_period_s_;
	/* Polling period to check whether to send new files in seconds.
	 */
	std::chrono::milliseconds prop_delay_ms_;
	/* Delay between taking a snapshot and crawling through the directory
	 * in milliseconds.
	 */
	fs::path last_rctime_path_;
	/* Metadata path to store last_rctime.dat in.
	 */
	std::string exec_bin_;
	/* File syncing program name.
	 */
	std::string exec_flags_;
	/* Flags and extra args for program.
	 */
	
	// source
	fs::path base_path_;
	/* Path to CephFS directory to back up.
	 */
	
	// target
	std::string remote_user_;
	/* SSH user to log in as. Optional.
	 */
	std::string remote_host_;
	/* Host of destination directory. Optional.
	 */
	fs::path remote_directory_;
	/* Backup directory to place files in. Optional.
	 */
public:
	Config(const fs::path &config_path, const ConfigOverrides &config_overrides);
	/* Construct Config object and load in configuration settings from disk.
	 */
	~Config() = default;
	/* Default destructor.
	 */
	void init_config_file(const fs::path &config_path) const;
	/* When config file DNE, this is called to create and initialize one.
	 */
	void override_fields(const ConfigOverrides &config_overrides);
	/* Replaces configuration fields with those of config_overrides
	 * as long as their overridden flags are true.
	 */
	void verify(const fs::path &config_path) const;
	/* Verifies that all config fields are valid.
	 */
	void dump(void) const;
	/* Prints configuration settings as a log message.
	 */
};

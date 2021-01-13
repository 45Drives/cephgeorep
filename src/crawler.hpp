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

#include "config.hpp"
#include "rctime.hpp"
#include <list>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class Crawler{
private:
	Config config_;
	/* holds user configuration options
	 */
	std::list<fs::path> file_list_;
	/* lsit of files to send to remote backup
	 */
	LastRctime last_rctime_;
	/* timestamp of last sync
	 */
	fs::path base_path_;
public:
	Crawler(fs::path config_path, ConfigOverrides overrides);
	/* calls config constructor with
	 * config_path and overrides,
	 * initialises last_rctime_ and payload_bytes_
	 */
	~Crawler(void) = default;
	/* default destructor
	 */
	void reset(void);
	/* clear file_list_ and set
	 * payload_bytes_ to 0
	 */
	void poll_base(bool loop);
	/* main loop of program
	 * polls for change in root sync path,
	 * if change detected, a snapshot is
	 * taken and the file queuing 
	 * search is triggered
	 */
	fs::path create_snap(const timespec &rctime) const;
	/* create snapshot in base directory,
	 * return snapshot path
	 */
	void trigger_search(const fs::path &path, uintmax_t &total_bytes);
	/* queues newly modified/created files
	 * into file_list_, keeps tally of filesize in
	 * total_bytes
	 */
	bool ignore_entry(const fs::directory_entry &entry) const;
	/* returns true if file should not be queued or directory should
	 * not be recursed into
	 */
	void find_new_files_recursive(fs::path current_path, const fs::path &snap_root, uintmax_t &total_bytes);
	/* recursive DFS on directory tree to queue files,
	 * keeps tally of filesize in total_bytes
	 */
	void delete_snap(const fs::path &snap_root) const;
	/* deletes snapshot directory
	 */
};

// ----------------

// void initDaemon(void);
// // calls loadConfig(), enables signal handlers, asserts that path to sync exists
// 
// void pollBase(fs::path path, bool loop);
// // main loop, check for change in rctime and launch crawler
// 
// void crawler(fs::path path, std::list<fs::path> &queue, const fs::path &snapdir, uintmax_t &total_bytes);
// // recursive directory crawler. Returns new files as boost::filesystem::path in queue.
// 
// bool checkForChange(const fs::path &path, const timespec &last_rctime, timespec &rctime);
// // returns true if subdir rctime or file mtime > last_rctime. Updates rctime with highest new rctime.
// 
// fs::path takesnap(const timespec &rctime);
// // take Ceph snapshot by creating a .snap/<rctime> directory
// 
// bool deletesnap(fs::path path);
// // remove snapshot directory path. Returns true if successful, false if path DNE

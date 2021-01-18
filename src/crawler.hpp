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

#include "exec.hpp"
#include "config.hpp"
#include "rctime.hpp"
#include <list>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class Crawler{
private:
	Config config_;
	/* Holds user configuration options.
	 */
	std::list<fs::path> file_list_;
	/* List of files to send to remote backup.
	 */
	LastRctime last_rctime_;
	/* Timestamp of last sync.
	 */
	fs::path base_path_;
	/* Local source directory.
	 */
	Syncer syncer;
public:
	Crawler(const fs::path &config_path, size_t envp_size, const ConfigOverrides &config_overrides);
	/* Calls config constructor with
	 * config_path and overrides,
	 * initialises last_rctime_ and payload_bytes_.
	 */
	~Crawler(void) = default;
	/* Default destructor.
	 */
	void reset(void);
	/* Clear file_list_ and set
	 * payload_bytes_ to 0.
	 */
	void poll_base(bool seed, bool dry_run);
	/* Main loop of program.
	 * Polls for change in root sync path,
	 * if change detected, a snapshot is
	 * taken and the file queuing
	 * search is triggered.
	 */
	fs::path create_snap(const timespec &rctime) const;
	/* Create snapshot in base directory,
	 * return snapshot path.
	 */
	void trigger_search(const fs::path &path, uintmax_t &total_bytes);
	/* Queues newly modified/created files
	 * into file_list_, keeps tally of filesize in
	 * total_bytes.
	 */
	bool ignore_entry(const fs::directory_entry &entry) const;
	/* Returns true if file should not be queued or directory should
	 * not be searched.
	 */
	void find_new_files_recursive(fs::path current_path, const fs::path &snap_root, uintmax_t &total_bytes);
	/* Recursive DFS on directory tree to queue files.
	 * Keeps tally of filesize in total_bytes.
	 */
	void delete_snap(const fs::path &snap_root) const;
	/* Deletes snapshot directory.
	 */
};

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

#include "exec.hpp"
#include "config.hpp"
#include "rctime.hpp"
#include "concurrent_queue.hpp"
#include "file.hpp"
#include <atomic>
#include <mutex>
#include <list>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class Crawler{
private:
	Config config_;
	/* Holds user configuration options.
	 */
	std::mutex file_list_mt_;
	/* Make file_list_ thread safe for insertion.
	 */
	std::vector<File> file_list_;
	/* List of files to send to remote backup.
	 */
	LastRctime last_rctime_;
	/* Timestamp of last sync.
	 */
	fs::path base_path_;
	/* Local source directory.
	 */
	fs::path snap_path_;
	/* Path to current snapshot.
	 */
	Syncer syncer;
	/* Controls executing the sync program.
	 */
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
	void poll_base(bool seed, bool dry_run, bool set_rctime, bool oneshot);
	/* Main loop of program.
	 * Polls for change in root sync path,
	 * if change detected, a snapshot is
	 * taken and the file queuing
	 * search is triggered.
	 */
	void create_snap(const timespec &rctime);
	/* Create snapshot in base directory
	 */
	void trigger_search(const boost::filesystem::path& snap_path, uintmax_t& total_bytes);
	/* Queues newly modified/created files
	 * into file_list_, keeps tally of filesize in
	 * total_bytes.
	 */
	bool ignore_entry(const fs::path &path) const;
	/* Returns true if file should not be queued or directory should
	 * not be searched.
	 */
	void find_new_files_recursive(fs::path current_path, const fs::path &snap_root, uintmax_t &total_bytes);
	/* Recursive DFS on directory tree to queue files.
	 * Keeps tally of filesize in total_bytes.
	 * This is used if threads == 1.
	 */
	void find_new_files_mt_bfs(ConcurrentQueue<fs::path> &queue, const fs::path &snap_root, std::atomic<uintmax_t> &total_bytes, std::atomic<int> &threads_running);
	/* Worker thread function to do multithreaded BFS on directory tree to queue files.
	 * Keeps tally of filesize in total_bytes.
	 * This is used if threads > 1.
	 */
	void delete_snap(void) const;
	/* Deletes snapshot directory.
	 */
	void write_last_rctime(void) const;
	/* Call last_rctime_.write_last_rctime().
	 */
};

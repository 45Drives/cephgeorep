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

#include "alert.hpp"
#include "config.hpp"
#include "file.hpp"
#include <vector>
#include <list>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#define SSH_FAIL 255
#define NOT_INSTALLED 127
#define SUCCESS 0
#define PERM_DENY 23

class Syncer;

class SyncProcess{
private:
	int id_;
	/* Integral ID for each process to be used while printing
	 * log messages.
	 */
	pid_t pid_;
	/* PID of process.
	 */
	size_t start_arg_sz_;
	/* Number of bytes contained in the environment and in the
	 * executable path and flags.
	 */
	size_t max_arg_sz_;
	/* Maximum number of bytes allowed in environment and argv of called
	 * process. Roughly 1/4 of stack.
	 */
	size_t curr_arg_sz_;
	/* Keeps track of bytes in argument vector.
	 */
	uintmax_t max_bytes_sz_;
	/* Number of bytes of files on disk for each process to sync.
	 * Equal to total bytes divided by number of processes.
	 */
	uintmax_t curr_bytes_sz_;
	/* Keeps track of payload size.
	 */
	std::string exec_bin_;
	/* File syncing program name.
	 */
	std::string exec_flags_;
	/* Flags and extra args for program.
	 */
	std::vector<std::string>::iterator &destination_;
	/* [[<user>@]<host>:][<destination path>]
	 */
	std::string sending_to_;
	/* For printing failures after iterator changes.
	 */
	std::vector<fs::path> files_;
	/* List of files to sync.
	 */
	std::vector<char *> garbage_;
	/* Garbage cleanup for allocated c-strings.
	 */
public:
	SyncProcess(Syncer *parent, uintmax_t max_bytes_sz);
	/* Constructor. Grabs members from parent pointer.
	 */
	~SyncProcess();
	/* Destructor. Frees memory in garbage vector before returning.
	 */
	int id() const;
	/* Return ID of sync process.
	 */
	pid_t pid() const;
	/* Return PID of sync process.
	 */
	void set_id(int id);
	/* Assign integral ID.
	 */
	uintmax_t payload_sz(void) const;
	/* Return number of bytes in file payload.
	 */
	uintmax_t payload_count(void) const;
	/* Return number of files in payload.
	 */
	void add(const File &file);
	/* Add one file to the payload.
	 * Incrememnts curr_arg_sz_ and curr_bytes_sz_ accordingly.
	 */
	bool full_test(const File &file) const;
	/* Returns true if curr_arg_sz_ or curr_bytes_sz_ exceed the maximums
	 * if file were to be added.
	 */
	bool large_file(const File &file) const;
	/* Check if file size is larger than maximum payload by itself.
	 * If this wasn't checked, large files would never sync.
	 */
	void consume(std::vector<File> &queue);
	/* Pop files from queue and push into files_ vector until full.
	 */
	void consume_one(std::vector<File> &queue);
	/* Only pop and push one file.
	 */
	void sync_batch(void);
	/* Fork and execute sync program with file batch.
	 */
	void clear_file_list(void);
	/* Clear files_ and reset curr_arg_sz_ and curr_bytes_sz_ to
	 * start_arg_sz_ and 0, respectively.
	 */
	bool payload_empty(void) const;
	/* Returns files_.empty().
	 */
	const std::string &destination(void) const;
	/* Returns sending_to_.
	 */
};

class Syncer{
	friend class SyncProcess;
	/* Allow access to members for SyncProcess's constructor.
	 */
private:
	int nproc_;
	/* Number of processes to launch. Defined in configuration file or
	 * in command line argument, but is limited to [1, # of files].
	 */
	size_t max_arg_sz_;
	/* Maximum number of bytes allowed in environment and argv of called
	 * process. Roughly 1/4 of stack.
	 */
	size_t start_arg_sz_;
	/* Number of bytes contained in the environment and in the
	 * executable path and flags.
	 */
	std::string exec_bin_;
	/* File syncing program name.
	 */
	std::string exec_flags_;
	/* Flags and extra args for program.
	 */
	std::vector<std::string> destinations_;
	/* [[<user>@]<host>:][<destination path>]
	 */
	std::vector<std::string>::iterator destination_;
public:
	Syncer(size_t envp_size, const Config &config);
	/* Determines max_arg_sz_, start_arg_sz_, and constructs destination_.
	 */
	~Syncer(void) = default;
	/* Default destructor.
	 */
	std::string construct_destination(std::string remote_user, std::string remote_host, fs::path remote_directory) const;
	/* Create [<user>@][<host>:][<destination path>] string.
	 */
	size_t get_max_arg_sz(void) const;
	/* Determine max_arg_sz_ from stack limits
	 */
	void launch_procs(std::vector<File> &queue, uintmax_t total_bytes);
	/* Creates SyncProcesses and distributes files across each one. Assigns each process an ID then launches
	 * them in parallel. Waits for processes to return and relaunches if there are files remaining.
	 */
	void distribute_files(std::vector<File> &queue, std::list<SyncProcess> &procs) const;
	/* Round robin distribution of files until all processes are full.
	 */
};

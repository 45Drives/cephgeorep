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

#include <vector>
#include <string>

class Syncer;
class File;

struct ExecError{
	bool exec_failed_ = false;
	int errno_ = 0;
};

class SyncProcess{
	friend class Syncer;
private:
	int id_;
	/* Integral ID for each process to be used while printing
	 * log messages. Also starting point of file_itr_.
	 */
	int inc_;
	/* Amount to increment the file list iterator by.
	 */
	pid_t pid_;
	/* PID of process.
	 */
	size_t start_payload_sz_;
	/* Count of elements in argv for sync process without any files added
	 */
	size_t max_mem_usage_;
	/* Maximum number of bytes allowed in environment and argv of called
	 * process.
	 */
	size_t start_mem_usage_;
	/* Number of bytes contained in the environment and in the
	 * executable path and flags.
	 */
	size_t curr_mem_usage_;
	/* Keeps track of bytes in argument vector.
	 */
	uintmax_t curr_payload_bytes_;
	/* Keeps track of payload size.
	 */
	int pipefd_[2];
	/* Pipe for logging errors of sync process.
	 */
	std::vector<std::string>::iterator &destination_;
	/* [[<user>@]<host>:][<destination path>]
	 */
	std::string sending_to_;
	/* For printing failures after iterator changes.
	 */
	std::vector<File>::iterator file_itr_;
	/* Iterator to list of files to sync.
	 */
	std::vector<char *> payload_;
	/* argv for sync process.
	 */
	ExecError *exec_error_;
	/* Struct pointer for returning errno from exec fail.
	 */
public:
	SyncProcess(Syncer *parent, int id, int nproc, std::vector<File> &queue);
	/* Constructor. Grabs members from parent pointer.
	 */
	~SyncProcess();
	/* Destructor.
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
	void add(const std::vector<File>::iterator &itr);
	/* Add one file to the payload.
	 * Incrememnts curr_mem_usage_ and curr_payload_bytes_ accordingly.
	 */
	bool full_test(const File &file) const;
	/* Returns true if curr_mem_usage_ exceeds the maximum
	 * if file were to be added.
	 */
	void consume(std::vector<File> &queue);
	/* Push c string pointers into payload_ vector until memory
	 * usage is full or end of queue.
	 */
	void change_destination(void);
	/* Pop last item in payload_ (destination) and replace with new destination.
	 */
	void sync_batch(void);
	/* Fork and execute sync program with file batch.
	 */
	void reset(void);
	/* free memory taken by path c-strings
	 * clear payload from start_payload_sz_ to end
	 * set curr_mem_usage_ to start_mem_usage_
	 * set curr_payload_bytes_ to 0
	 */
	bool done(const std::vector<File> &queue) const;
	/* Returns file_itr_ >= queue.end().
	 */
	const std::string &destination(void) const;
	/* Returns sending_to_.
	 */
	void dump_argv(int error) const;
	/* Print errno, strerror(errno), and payload_ to a log file
	 * when execution fails.
	 */
	void log_errors(void) const;
	/* Print contents of pipe to log file.
	 */
};

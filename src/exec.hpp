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

#include "alert.hpp"
#include "config.hpp"
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
	pid_t pid_;
	size_t start_arg_sz_;
	size_t max_arg_sz_;
	size_t curr_arg_sz_;
	uintmax_t max_bytes_sz_;
	uintmax_t curr_bytes_sz_;
	std::string exec_bin_;
	std::string exec_flags_;
	std::string destination_;
	std::vector<fs::path> files_;
	std::vector<char *> garbage_;
public:
	SyncProcess(const Syncer *parent, uintmax_t max_bytes_sz); //size_t max_arg_sz, size_t start_arg_sz, uintmax_t max_bytes_sz, const std::string &destination);
	~SyncProcess();
	pid_t pid() const;
	void set_id(int id);
	uintmax_t payload_sz(void) const;
	void add(const fs::path &file);
	bool full_test(const fs::path &file) const;
	bool large_file(const fs::path &file) const;
	void consume(std::list<fs::path> &queue);
	void consume_one(std::list<fs::path> &queue);
	void sync_batch(void);
	// fork and execute sync program with file batch
	void clear_file_list(void);
};

class Syncer{
	friend class SyncProcess;
private:
	int nproc_;
	size_t max_arg_sz_;
	size_t start_arg_sz_;
	std::string exec_bin_;
	std::string exec_flags_;
	std::string destination_;
public:
	Syncer(size_t envp_size, const Config &config);
	~Syncer(void) = default;
	void construct_destination(std::string remote_user, std::string remote_host, fs::path remote_directory);
	size_t get_max_arg_sz(void) const;
	void launch_procs(std::list<fs::path> &queue, uintmax_t total_bytes) const;
	void distribute_files(std::list<fs::path> &queue, std::list<SyncProcess> &procs) const;
};

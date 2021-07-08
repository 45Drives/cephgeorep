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

#define SSH_FAIL 255
#define NOT_INSTALLED 127
#define SUCCESS 0
#define PARTIAL_XFR 23

#ifndef MEM_LIM_HEADROOM
#define MEM_LIM_HEADROOM 2048 // POSIX suggests 2048 bytes of headroom for modifying env
#endif

#include <list>
#include <vector>
#include <string>

class SyncProcess;
class Config;
class File;

class Syncer{
	friend class SyncProcess;
	/* Allow access to members for SyncProcess's constructor.
	 */
private:
	int nproc_;
	/* Number of processes to launch. Defined in configuration file or
	 * in command line argument, but is limited to [1, # of files].
	 */
	size_t max_mem_usage_;
	/* Maximum number of bytes allowed in environment and argv of called
	 * process.
	 */
	size_t start_mem_usage_;
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
	/* Iterator to current destination.
	 */
	std::vector<char *> start_payload_;
	/* Start of argv to pass to exec bin.
	 */
	std::vector<char *> garbage_;
	/* For cleanup on destruction.
	 */
public:
	Syncer(size_t envp_size, const Config &config);
	/* Determines max_arg_sz_, start_arg_sz_, and constructs destination_.
	 */
	~Syncer(void);
	/* Destructor.
	 */
	std::string construct_destination(std::string remote_user, std::string remote_host, std::string remote_directory) const;
	/* Create [<user>@][<host>:][<destination path>] string.
	 */
	size_t get_mem_limit(size_t envp_size) const;
	/* Determine max_arg_sz_ from stack limits
	 */
	void launch_procs(std::vector<File> &queue);
	/* Creates SyncProcesses and distributes files across each one. Assigns each process an ID then launches
	 * them in parallel. Waits for processes to return and relaunches if there are files remaining.
	 */
	void distribute_files(std::vector<File> &queue, std::list<SyncProcess> &procs) const;
	/* Round robin distribution of files until all processes are full.
	 */
};

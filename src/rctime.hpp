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

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#define XATTR_SIZE 1024

class LastRctime{
	/* holds timestamp of previous file sync
	 * to determine whether files are newly
	 * changed since last sync
	 */
private:
	timespec last_rctime_;
	/* timestamp of last sync
	 */
	fs::path last_rctime_path_;
	/* location to store last_rctime_
	 * on disk
	 */
public:
	LastRctime(const fs::path &last_rctime_path);
	/* tries to read last_rctime_ from disk
	 * if not on disk, initializes to 0.0
	 */
	~LastRctime(void);
	/* calls write_last_rctime()
	 */
	void write_last_rctime(void) const;
	/* writes last_rctime_ to disk
	 */
	void init_last_rctime(void) const;
	/* creates file to store last_rctime_
	 * and initializes to 0.0
	 */
	bool is_newer(const fs::path &path) const;
	/* calls get_rctime on path, returns true
	 * if rctime of path is > last_rctime_
	 */
	timespec get_rctime(const fs::path &path) const;
	/* returns timespec of mtime if path is a file
	 * or ceph.dir.rctime if path is a directory
	 */
	bool check_for_change(const fs::path &path, timespec &new_rctime) const;
	/* checks rctimes and mtimes of each entry in the root directory
	 * against last_rctime_ and returns true if there are new changes,
	 * returns lowest rctime or mtime above last_rctime_ by reference
	 * in new_rctime.
	 */
	void update(const timespec &new_rctime);
	/* copies new_rctime into last_rctime_
	 */
	friend std::string &operator+(std::string lhs, const LastRctime &rhs);
	/* returns rhs concatenated onto end of string
	 */
	friend std::string &operator+(const LastRctime &lhs, std::string rhs);
	/* returns lhs concatenated onto beginning of string
	 */
};


//---------------------------------------------------------

// extern timespec last_rctime;
// 
// timespec loadLast_rctime(void);
// // reads previously saved rctime from disk
// // file format = rctime.tv_sec + '.' + rctime.tv_nsec + '\n'
// 
// void writeLast_rctime(const timespec &rctime);
// // writes rctime to last_rctime.dat
// 
// void init_last_rctime(void);
// // creates last_rctime.dat file and initializes contents to "0.0\n".
// 
// timespec get_rctime(const fs::path &path);
// // returns ceph.dir.rctime of directory path or mtime of file
// 
// // timespec operators:
// std::string &operator+(std::string lhs, timespec rhs);
// std::string &operator+(timespec lhs, std::string rhs);
// bool operator>(const timespec &lhs, const timespec &rhs);
// bool operator==(const timespec &lhs, const timespec &rhs);
// bool operator>=(const timespec &lhs, const timespec &rhs);
// bool operator<(const timespec &lhs, const timespec &rhs);
// bool operator<=(const timespec &lhs, const timespec &rhs);
// std::ostream &operator<<(std::ostream &stream, const struct timespec &rhs); 

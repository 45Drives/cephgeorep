/*
    Copyright (C) 2019 Joshua Boudreau
    
    This file is part of cephgeorep.

    cephgeorep is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    cephgeorep is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#define XATTR_SIZE 1024
#define ERR -1

extern timespec last_rctime;

timespec loadLast_rctime(void);
// reads previously saved rctime from disk
// file format = rctime.tv_sec + '.' + rctime.tv_nsec + '\n'

void writeLast_rctime(const timespec &rctime);
// writes rctime to last_rctime.dat

void init_last_rctime(void);
// creates last_rctime.dat file and initializes contents to "0.0\n".

timespec get_rctime(const fs::path &path);
// returns ceph.dir.rctime of directory path or mtime of file

// timespec operators:
std::string &operator+(std::string lhs, timespec rhs);
std::string &operator+(timespec lhs, std::string rhs);
bool operator>(const timespec &lhs, const timespec &rhs);
bool operator==(const timespec &lhs, const timespec &rhs);
bool operator>=(const timespec &lhs, const timespec &rhs);
bool operator<(const timespec &lhs, const timespec &rhs);
bool operator<=(const timespec &lhs, const timespec &rhs);
std::ostream &operator<<(std::ostream &stream, const struct timespec &rhs); 

#pragma once

#include <ctime>
#include <boost/filesystem.hpp>

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

timespec get_rctime(const boost::filesystem::path &path);
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

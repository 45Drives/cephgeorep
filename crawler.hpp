#pragma once

#include <boost/filesystem.hpp>
#include <vector>

enum FilesOrDirs{FILES, DIRS, BOTH};

void pollBase(boost::filesystem::path path);
// main loop

void crawler(boost::filesystem::path path, std::vector<boost::filesystem::path> &queue);
// recursive directory crawler. Returns new files.

bool checkForChange(const boost::filesystem::path &path, const timespec &last_rctime, timespec &rctime);
// returns true if dir rctime or file mtime > last_rctime. Updates rctime with highest new rctime.

int count(boost::filesystem::path path, FilesOrDirs choice);
// returns number of files or directories in a specific directory path

void read_directory(boost::filesystem::path path,
std::vector<boost::filesystem::path> &q, FilesOrDirs choice);
// reads contents of given directory and enques at end of q

boost::filesystem::path takesnap(const timespec &rctime);
// take Ceph snapshot by creating a .snap/<rctime> directory

bool deletesnap(boost::filesystem::path path);
// remove snapshot directory path. Returns true if successful, false if path DNE

#pragma once

#include <boost/filesystem.hpp>
#include <vector>

namespace fs = boost::filesystem;

enum FilesOrDirs{FILES, DIRS, BOTH};

void pollBase(fs::path path);
// main loop

void crawler(fs::path path, std::vector<fs::path> &queue, const fs::path &snapdir);
// recursive directory crawler. Returns new files.

bool checkForChange(const fs::path &path, const timespec &last_rctime, timespec &rctime);
// returns true if dir rctime or file mtime > last_rctime. Updates rctime with highest new rctime.

int count(fs::path path, FilesOrDirs choice);
// returns number of files or directories in a specific directory path

void read_directory(fs::path path,
std::vector<fs::path> &q, FilesOrDirs choice);
// reads contents of given directory and enques at end of q

fs::path takesnap(const timespec &rctime);
// take Ceph snapshot by creating a .snap/<rctime> directory

bool deletesnap(fs::path path);
// remove snapshot directory path. Returns true if successful, false if path DNE

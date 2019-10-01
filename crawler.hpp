#pragma once

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

enum FilesOrDirs{FILES, DIRS, BOTH};

void initDaemon(void);
// calls loadConfig(), enables signal handlers, asserts that path to sync exists

void pollBase(fs::path path);
// main loop, check for change in rctime and launch crawler

void crawler(fs::path path, std::vector<fs::path> &queue, const fs::path &snapdir);
// recursive directory crawler. Returns new files as boost::filesystem::path in queue.

bool checkForChange(const fs::path &path, const timespec &last_rctime, timespec &rctime);
// returns true if subdir rctime or file mtime > last_rctime. Updates rctime with highest new rctime.

fs::path takesnap(const timespec &rctime);
// take Ceph snapshot by creating a .snap/<rctime> directory

bool deletesnap(fs::path path);
// remove snapshot directory path. Returns true if successful, false if path DNE

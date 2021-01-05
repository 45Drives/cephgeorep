/*
    Copyright (C) 2019-2020 Joshua Boudreau
    
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

void initDaemon(void);
// calls loadConfig(), enables signal handlers, asserts that path to sync exists

void pollBase(fs::path path, bool loop);
// main loop, check for change in rctime and launch crawler

void crawler(fs::path path, std::list<fs::path> &queue, const fs::path &snapdir);
// recursive directory crawler. Returns new files as boost::filesystem::path in queue.

bool checkForChange(const fs::path &path, const timespec &last_rctime, timespec &rctime);
// returns true if subdir rctime or file mtime > last_rctime. Updates rctime with highest new rctime.

fs::path takesnap(const timespec &rctime);
// take Ceph snapshot by creating a .snap/<rctime> directory

bool deletesnap(fs::path path);
// remove snapshot directory path. Returns true if successful, false if path DNE

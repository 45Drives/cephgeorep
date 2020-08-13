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

#include <boost/system/error_code.hpp>

extern boost::system::error_code ec; // error code for system fucntions

#define NUM_ERRS 13

enum {
  OPEN_CONFIG, PATH_CREATE, READ_RCTIME, READ_MTIME, READ_FILES_DIRS,
  REMOVE_SNAP, FORK, LAUNCH_RSYNC, WAIT_RSYNC, NO_RSYNC, UNK_RSYNC_ERR,
  SND_DIR_DNE, NO_PERM
};

void error(int err, boost::system::error_code ec_ = {1,boost::system::generic_category()});
// print error to std::cerr and exit with error code 1

void Log(std::string msg, int lvl);
// print msg to std::cout given config.log_level >= lvl

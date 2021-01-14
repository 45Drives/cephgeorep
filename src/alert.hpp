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

#include <iostream>

class Logger{
private:
	int log_level_;
public:
	Logger(int log_level){
		log_level_ = log_level;
	}
	void message(const std::string &msg, int lvl){
		if(log_level_ >= lvl) std::cout << msg << std::endl;
	}
	void warning(const std::string &msg){
		std::cout << "Warning: " << msg << std::endl;
	}
	void error(const std::string &msg){
		std::cout << "Error: " << msg << std::endl;
		exit(EXIT_FAILURE);
	}
};

namespace Logging{
	extern Logger log;
}

// ----------------------------------------------

// #include <boost/system/error_code.hpp>
// 
// extern boost::system::error_code ec; // error code for system fucntions
// 
// #define NUM_ERRS 13
// 
// enum {
// 	OPEN_CONFIG, PATH_CREATE, READ_RCTIME, READ_MTIME, READ_FILES_DIRS,
// 	REMOVE_SNAP, FORK, LAUNCH_RSYNC, WAIT_RSYNC, NO_RSYNC, UNK_RSYNC_ERR,
// 	SND_DIR_DNE, NO_PERM
// };
// 
// void warning(int err);
// // print error to std::cerr but don't exit
// 
// void error(int err, boost::system::error_code ec_ = {1,boost::system::generic_category()});
// // print error to std::cerr and exit with error code 1
// 
// void Log(std::string msg, int lvl);
// // print msg to std::cout given config.log_level >= lvl
// 
// void usage(void);
// // print CLI flag descriptions

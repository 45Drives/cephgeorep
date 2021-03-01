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

#include "alert.hpp"
#include "signal.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace Logging{
	Logger log(1);
}

Logger::Logger(int log_level){
	log_level_ = log_level;
}

void Logger::message(const std::string &msg, int lvl) const{
	if(log_level_ >= lvl) std::cout << msg << std::endl;
}

void Logger::warning(const std::string &msg) const{
	std::cerr << "Warning: " << msg << std::endl;
}

void Logger::error(const std::string &msg) const{
	std::cerr << "Error: " << msg << std::endl;
}

#define N_INDEX 9
std::string Logger::format_bytes(uintmax_t bytes) const{
	if(bytes == 0) return "0 B";
	std::stringstream formatted_ss;
	std::string units[N_INDEX] = {" B", " KiB", " MiB", " GiB", " TiB", " PiB", " EiB", " ZiB", " YiB"};
	int i = std::min(int(log(bytes) / log(1024.0)), N_INDEX - 1);
	double p = pow(1024.0, i);
	double formatted = double(bytes) / p;
	formatted_ss << std::fixed << std::setprecision(2) << formatted << units[i];
	return formatted_ss.str();
}

std::string Logger::rsync_error(int err) const{
	switch(err){
		case 0:
			return "Success";
		case 1:
			return "Syntax or usage error";
		case 2:
			return "Protocol incompatibility";
		case 3:
			return "Errors selecting input/output files, dirs";
		case 4:
			return "Requested action not supported";
		case 5:
			return "Error starting client-server protocol";
		case 6:
			return "Daemon unable to append to log-file";
		case 10:
			return "Error in socket I/O";
		case 11:
			return "Error in file I/O";
		case 12:
			return "Error in rsync protocol data stream";
		case 13:
			return "Errors with program diagnostics";
		case 14:
			return "Error in IPC code";
		case 20:
			return "Received SIGUSR1 or SIGINT";
		case 21:
			return "Some error returned by waitpid()";
		case 22:
			return "Error allocating core memory buffers";
		case 23:
			return "Partial transfer due to error";
		case 24:
			return "Partial transfer due to vanished source files";
		case 25:
			return "The --max-delete limit stopped deletions";
		case 30:
			return "Timeout in data send/receive";
		case 35:
			return "Timeout waiting for daemon connection";
		default:
			return "Unknown error: " + std::to_string(err);
	}
}

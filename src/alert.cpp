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

#include "alert.hpp"
#include <iostream>

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
void Logger::error(const std::string &msg, bool exit_) const{
	std::cerr << "Error: " << msg << std::endl;
	if(exit_) exit(EXIT_FAILURE);
}

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

#include "status.hpp"
#include "alert.hpp"
#include <fstream>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace Status{
	StatusSetter status{};
}

StatusSetter::StatusSetter(void){
	try{
		fs::create_directories(STATUS_PATH);
	}catch(const boost::system::error_code &ec){
		Logging::log.error("Failed to create status path (" STATUS_PATH "): " + ec.message());
		exit(EXIT_FAILURE);
	}
	f_.open(STATUS_PATH STATUS_FILE, std::fstream::out | std::fstream::trunc);
	f_ << 0 << std::endl;
	f_.close();
}

void StatusSetter::set(int status){
	f_.open(STATUS_PATH STATUS_FILE, std::fstream::out | std::fstream::trunc);
	f_ << status << std::endl;
	f_.close();
}

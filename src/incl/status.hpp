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

#pragma once

#include <fstream>

#define STATUS_PATH "/run/cephgeorep/"
#define STATUS_FILE "status"

class StatusSetter{
private:
	std::ofstream f_;
public:
	StatusSetter(void);
	void set(int status);
};

namespace Status{
	// Status code definitions for Prometheus exporter
	const int OK = 0;
	const int NOT_RUNNING = 1;
	const int HOST_DOWN = 2;
	const int ALL_HOSTS_DOWN = 3;
	
	extern StatusSetter status;
}

int make_status_file(void);

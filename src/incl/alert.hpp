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

#include <string>

class Logger{
private:
	int log_level_;
	/* Value from config file. Each log message
	 * passes a log level to check against this number.
	 * If the message's level is higher, it is printed.
	 */
public:
	explicit Logger(int log_level);
	/* Constructs Logger, assigning log_level to the
	 * internal log_level_.
	 */
	void message(const std::string &msg, int lvl) const;
	/* Print message to stdout if lvl >= log_level_.
	 * Use this for regular informational log messages.
	 */
	void warning(const std::string &msg) const;
	/* Print message to stderr prepended with "Warning: ".
	 * Use this for non-fatal errors.
	 */
	void error(const std::string &msg) const;
	/* Print message to stderr prepended with "Error: ".
	 */
	std::string format_bytes(uintmax_t bytes) const;
	/* Return bytes as string in base-1024 SI units.
	 */
	std::string rsync_error(int err) const;
	/* Translate rsync exit code to error message.
	 */
};

namespace Logging{
	extern Logger log;
	/* Global Logger object. Use Logging::log.<method> in source
	 * files including this header.
	 */
}

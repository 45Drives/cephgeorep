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

#include "rctime.hpp"

namespace signal_handling{
	extern const LastRctime *last_rctime_;
	/* Pointer to last_rctime_ member of Crawler.
	 * Used to hook Crawler.last_rctime_.write_last_rctime() into
	 * the signal handler for SIGINT, SIGQUIT, and SIGTERM.
	 */
}

void set_signal_handlers(const LastRctime *last_rctime);
/* Save pointer to crawler.last_rctime_ in signal_handling::last_rctime_
 * and call signal() to set up each signal to be handled by the
 * signal handler in signal.cpp.
 */

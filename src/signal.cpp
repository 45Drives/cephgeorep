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

#include "signal.hpp"
#include <csignal>

namespace signal_handling{
	const LastRctime *last_rctime_;
}

void sigint_hdlr(int signum){
	switch(signum){
		case SIGTERM:
		case SIGINT:
			// cleanup from termination
			signal_handling::last_rctime_->write_last_rctime();
			exit(EXIT_SUCCESS);
		default:
			exit(signum);
	}
}

void set_signal_handlers(const LastRctime *last_rctime){
	signal_handling::last_rctime_ = last_rctime;
	signal(SIGINT, sigint_hdlr);
	signal(SIGTERM, sigint_hdlr);
	signal(SIGQUIT, sigint_hdlr);
}

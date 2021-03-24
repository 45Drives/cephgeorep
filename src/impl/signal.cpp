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

#include "signal.hpp"
#include "crawler.hpp"
#include "status.hpp"
#include <csignal>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace signal_handling{
	const Crawler *crawler_ = nullptr;
}

void sig_hdlr(int signum){
	// cleanup from termination
	if(signal_handling::crawler_){
		signal_handling::crawler_->write_last_rctime();
		signal_handling::crawler_->delete_snap();
	}
	switch(signum){
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
			Status::status.set(Status::NOT_RUNNING);
			exit(EXIT_SUCCESS);
		default:
			Status::status.set(Status::EXIT_FAILED);
			exit(signum);
	}
}

void set_signal_handlers(const Crawler *crawler){
	signal_handling::crawler_ = crawler;
	signal(SIGINT, sig_hdlr);
	signal(SIGTERM, sig_hdlr);
	signal(SIGQUIT, sig_hdlr);
}

void signal_handling::error_cleanup(void){
	if(signal_handling::crawler_)
		signal_handling::crawler_->delete_snap();
}

void l::exit(int num, int status){
	signal_handling::error_cleanup();
	if(num == EXIT_FAILURE && status == Status::NOT_RUNNING)
		Status::status.set(Status::EXIT_FAILED);
	else
		Status::status.set(status);
	::exit(num);
}

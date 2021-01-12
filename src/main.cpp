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

/*
 * Dependancies: boost filesystem, rsync
 * 
 */

#include <iostream>

#include "config.hpp"
#include "rctime.hpp"
#include "crawler.hpp"
#include "alert.hpp"
#include <getopt.h>
#include <string>

int main(int argc, char *argv[], char *envp[]){
	int opt;
	int option_ind = 0;
	int rsync_nproc_override = 0;
	bool loop = true; // keep daemon running
	
	static struct option long_options[] = {
		{"config",       required_argument, 0, 'c'},
		{"help",         no_argument,       0, 'h'},
		{"verbose",      no_argument,       0, 'v'},
		{"quiet",        no_argument,       0, 'q'},
		{"seed",         no_argument,       0, 's'},
		{"nproc",        required_argument, 0, 'n'},
		{0, 0, 0, 0}
	};
	
	while((opt = getopt_long(argc, argv, "c:hvqsn:", long_options, &option_ind)) != -1){
		switch(opt){
			case 'c':
				config_path = optarg;
				break;
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				config.log_level = 2;
				break;
			case 'q':
				config.log_level = 0;
				break;
			case 's':
				loop = false; // run daemon once then exit
				break;
			case 'n':
				try{
					rsync_nproc_override = std::stoi(optarg);
				}catch(std::invalid_argument){
					std::cout << "Invalid number of processes." << std::endl;
					exit(EXIT_FAILURE);
				}
			case '?':
				break; // getopt_long prints errors
			default:
				abort();
		}
	}
	
	initDaemon();
	config.env_sz = find_env_size(envp);
	if(rsync_nproc_override) config.rsync_nproc = rsync_nproc_override;
	if(loop == false) last_rctime = {1}; // seed all files
	pollBase(config.sender_dir, loop);
	return 0;
}

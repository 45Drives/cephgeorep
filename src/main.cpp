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

#include "config.hpp"
#include "rctime.hpp"
#include "crawler.hpp"
#include "alert.hpp"
#include <getopt.h>
#include <string>
#include <iostream>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

inline size_t get_env_size(char *envp[]){
	size_t size = 0;
	while(*envp){
		size += strlen(*envp++) + 1;
		size += sizeof(char *);
	}
	size += 1; // null terminator
	return size;
}

int main(int argc, char *argv[], char *envp[]){
	int opt;
	int option_ind = 0;
	bool seed = false; // sync everything but don't loop
	fs::path config_path = DEFAULT_CONFIG_PATH;
	
	ConfigOverrides config_overrides;
	
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
				config_path = fs::path(optarg);
				break;
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				config_overrides.log_level_override = ConfigOverride<int>(2);
				break;
			case 'q':
				config_overrides.log_level_override = ConfigOverride<int>(0);
				break;
			case 's':
				seed = true; // run daemon once then exit, syncing everything
				break;
			case 'n':
				try{
					config_overrides.nproc_override = ConfigOverride<int>(std::stoi(optarg));
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
	
	Crawler crawler(config_path, get_env_size(enpv), config_overrides);
	crawler.poll_base(seed);
	
	return 0;
}

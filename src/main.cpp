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

/*
 * Dependancies: boost filesystem, rsync
 * 
 */

#include "config.hpp"
#include "crawler.hpp"
#include "alert.hpp"
#include <getopt.h>
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

inline void usage(){
	Logging::log.message(
		"cephfssyncd Copyright (C) 2019-2021 Josh Boudreau <jboudreau@45drives.com>\n"
		"This program is released under the GNU General Public License v2.1.\n"
		"See <https://www.gnu.org/licenses/> for more details.\n"
		"\n"
		"Usage:\n"
		"  cephfssyncd [ flags ]\n"
		"Flags:\n"
		"  -c --config </path/to/config> - pass alternate config path\n"
		"                                  default config: /etc/ceph/cephfssyncd.conf\n"
		"  -d --dry-run                  - print total files that would be synced\n"
		"                                  when combined with -v, files will be listed\n"
		"                                  exits after showing number of files\n"
		"  -h --help                     - print this message\n"
		"  -n --nproc <# of processes>   - number of sync processes to run in parallel\n"
		"  -q --quiet                    - set log level to 0\n"
		"  -s --seed                     - send all files to seed destination\n"
		"  -v --verbose                  - set log level to 2\n"
		, 1
	);
}

int main(int argc, char *argv[], char *envp[]){
	int opt;
	int option_ind = 0;
	bool seed = false; // sync everything but don't loop
	bool dry_run = false; // don't actually sync
	fs::path config_path = DEFAULT_CONFIG_PATH;
	
	ConfigOverrides config_overrides;
	
	static struct option long_options[] = {
		{"config",       required_argument, 0, 'c'},
		{"help",         no_argument,       0, 'h'},
		{"verbose",      no_argument,       0, 'v'},
		{"quiet",        no_argument,       0, 'q'},
		{"seed",         no_argument,       0, 's'},
		{"nproc",        required_argument, 0, 'n'},
		{"dry-run",      no_argument,       0, 'd'},
		{0, 0, 0, 0}
	};
	
	while((opt = getopt_long(argc, argv, "c:hvqsn:d", long_options, &option_ind)) != -1){
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
				}catch(const std::invalid_argument &){
					Logging::log.error("Invalid number of processes.");
				}
				break;
			case 'd':
				dry_run = true;
				break;
			case '?':
				break; // getopt_long prints errors
			default:
				abort();
		}
	}
	
	Crawler crawler(config_path, get_env_size(envp), config_overrides);
	crawler.poll_base(seed, dry_run);
	
	return 0;
}

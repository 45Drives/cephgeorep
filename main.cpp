/*
    Copyright (C) 2019-2020 Joshua Boudreau
    
    This file is part of cephgeorep.

    cephgeorep is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    cephgeorep is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.
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

int main(int argc, char *argv[], char *envp[]){
  if(argc > 1){
    if(argc > 2 && (strcmp(argv[1],"-c") == 0 || strcmp(argv[1],"--config") == 0)){
      config_path = std::string(argv[2]);
    }else{
      std::cerr << "Unknown argument: " << argv[1] << std::endl;
    }
  }
  initDaemon();
  config.env_sz = find_env_size(envp);
  pollBase(config.sender_dir);
  return 0;
}

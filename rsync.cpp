#include "rsync.hpp"
#include "config.hpp"
#include "alert.hpp"
#include <vector>
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <sys/wait.h>

#include <iostream>

namespace fs = boost::filesystem;

void launch_rsync(std::vector<fs::path> queue){
  pid_t pid;
  int status;
  std::vector<char *> argv;
  std::vector<std::string> args({"rsync","-a","--relative"});
  if(config.compress) args.push_back("-z");
  
  for(fs::path i : queue){
    args.push_back(i.c_str());
  }
  
  args.push_back(config.receiver_host + ":" + config.receiver_dir.string());
  
  for(std::vector<std::string>::iterator itr = args.begin(); itr != args.end(); ++itr){
    argv.push_back(&(*itr)[0]);
  }
  
  argv.push_back(NULL);
  
  Log("Launching rsync with " + std::to_string(queue.size()) + " files.", 1);
  
  pid = fork(); // create child process
  switch(pid){
  case -1:
    error(FORK);
    break;
  case 0: // child process
    pid = getpid();
    Log("rsync process created with PID " + std::to_string(pid), 2);
    execvp(argv[0],&argv[0]);
    error(LAUNCH_RSYNC);
    break;
  default: // parent process
    // wait for rsync to finish before removing snapshot
    if((pid = wait(&status)) < 0)
      error(WAIT_RSYNC);
    Log("rsync process exited with status " + std::to_string(WEXITSTATUS(status)),2);
    break;
  }
}

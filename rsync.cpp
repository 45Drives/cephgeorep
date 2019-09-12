#include "rsync.hpp"
#include "config.hpp"
#include "alert.hpp"
#include <vector>
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>
#include <chrono>

#include <iostream>

namespace fs = boost::filesystem;

void launch_rsync(std::vector<fs::path> queue){
  pid_t pid;
  int status;
  std::vector<char *> argv;
  std::vector<std::string> args{"rsync","-a","--relative"};
  if(config.compress) args.push_back("-z");
  
  for(fs::path i : queue){
    args.push_back(i.string());
  }
  
  args.push_back(config.receiver_host + ":" + config.receiver_dir.string());
  
  for(std::vector<std::string>::iterator itr = args.begin(); itr != args.end(); ++itr){
    argv.push_back(&(*itr)[0]);
  }
  
  argv.push_back(NULL);
  
  while(WEXITSTATUS(status) == SSH_FAIL){
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
    switch(WEXITSTATUS(status)){
    case SUCCESS:
      break;
    case SSH_FAIL:
      Log("rsync failed to connect to remote backup server. "
      "Is the server running and connected to your network?",0);
      Log("Trying again in 25 seconds.",0);
      std::this_thread::sleep_for(std::chrono::seconds(25));
      break;
    case NOT_INSTALLED:
      error(NO_RSYNC);
      break;
    default:
      error(UNK_RSYNC_ERR);
      break;
    }
  }
}

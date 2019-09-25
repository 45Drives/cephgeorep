#include "rsync.hpp"
#include "config.hpp"
#include "alert.hpp"
#include <vector>
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>
#include <chrono>
#include <string>
#include <cstring>

namespace fs = boost::filesystem;

void launch_rsync(std::vector<fs::path> queue){
  pid_t pid;
  int status;
  std::vector<char *> argv{(char *)"rsync",(char *)"-a",(char *)"--relative"};
  if(config.compress) argv.push_back((char *)"-z");
  
  for(fs::path i : queue){
    char *str = new char[i.string().length()+1];
    std::strcpy(str,i.c_str());
    argv.push_back(str);
  }
  
  // push back host:/path/to/backup
  argv.push_back(config.rsync_remote_dest);
  
  argv.push_back(NULL);
  
  do{
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
  }while(WEXITSTATUS(status) == SSH_FAIL);
}

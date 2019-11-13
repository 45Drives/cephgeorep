#include "exec.hpp"
#include "config.hpp"
#include "alert.hpp"
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>
#include <chrono>

namespace fs = boost::filesystem;

void launch_syncBin(std::vector<fs::path> queue){
  pid_t pid;
  int status;
  std::vector<char *> argv;
  std::vector<char *> garbage;
  argv.push_back((char *)config.execBin.c_str());
  
  std::string binStr(config.execBin);
  
  boost::tokenizer<boost::char_separator<char>> tokens(config.execFlags, boost::char_separator<char>(" "));
  for(boost::tokenizer<boost::char_separator<char>>::iterator itr = tokens.begin(); itr != tokens.end(); ++itr){
    char *flag = new char[(*itr).length()+1];
    strcpy(flag,(*itr).c_str());
    argv.push_back(flag);
    garbage.push_back(flag);
  }
  
  for(fs::path i : queue){
    char *path = new char[i.string().length()+1];
    std::strcpy(path,i.c_str());
    argv.push_back(path);
    garbage.push_back(path);
  }
  
  // push back host:/path/to/backup
  if(config.sync_remote_dest[0] != '\0') argv.push_back(config.sync_remote_dest);
  
  argv.push_back(NULL);
  
  do{
    Log("Launching " + binStr + " " + config.execFlags + " with " + std::to_string(queue.size()) + " files.", 1);
    
    pid = fork(); // create child process
    switch(pid){
    case -1:
      error(FORK);
      break;
    case 0: // child process
      pid = getpid();
      Log(binStr + " process created with PID " + std::to_string(pid), 2);
      execvp(argv[0],&argv[0]);
      error(LAUNCH_RSYNC);
      break;
    default: // parent process
      // wait for rsync to finish before removing snapshot
      if((pid = wait(&status)) < 0)
        error(WAIT_RSYNC);
      Log(binStr + " process exited with status " + std::to_string(WEXITSTATUS(status)),2);
      break;
    }
    switch(WEXITSTATUS(status)){
    case SUCCESS:
      Log("Done.",1);
      break;
    case SSH_FAIL:
      Log(binStr + " failed to connect to remote backup server.\n"
      "Is the server running and connected to your network?\n"
      "Trying again in 25 seconds.",0);
      std::this_thread::sleep_for(std::chrono::seconds(25));
      break;
    case NOT_INSTALLED:
      error(NO_RSYNC);
      break;
    case PERM_DENY:
      error(NO_PERM);
      break;
    default:
      error(UNK_RSYNC_ERR);
      break;
    }
  }while(WEXITSTATUS(status) == SSH_FAIL);
  
  // garbage cleanup
  for(auto i : garbage){
    delete i;
  }
}

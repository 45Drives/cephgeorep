/*
	
	CEPH File System Sync Daemon
	
	Created: Joshua Boudreau	2019-01-08
	
	Ported from BASH script written by Brett Kelly
	
	Note: Must compile with C++11
	
*/

#define CONF_DIR "/etc/ceph/"
#define LAST_RCTIME conf["LAST_RCTIME_DIR"] + "last_rctime"
#define SNAP_NAME "georepsnap"
#define XATTR_SIZE 10000
#define ARG_SIZE 1000
#define TESTING false

#include <iostream>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fstream>
#include <unistd.h>
#include <map>
#include <string>
#include <cstring>
#include <vector>
#include <dirent.h>	//	read dir
#include <algorithm>
#include <ctime>
#include <thread>
#include <sstream>
using namespace std;

// Function declarations

map<string,string> LoadConfig(string filename);
void Log(string message, int lvl);
timespec getrctime(const string & path);
int getsubdirs(const string & path);
int getfiles(const string & path);
string takesnap(const timespec & rctime);
void traversedir(const string & path, const timespec & rctime_0, vector<string> & syncq);
void read_directory(const string & name, vector<string> & v);
bool is_dir(const char* path);
int rsync(const string & SNAP_DIR, vector<string> & syncq);
int exec(const char * programPath, char * const argv[], const string & SNAP_DIR);
bool checkrootforchange(const string & path, timespec & RCTIME_1);
void writelast_rctime(const timespec & rctime);
timespec readlast_rctime(void);
void deletesnap(const timespec & rctime);
bool operator >(const timespec& lhs, const timespec& rhs);
bool operator <(const timespec& lhs, const timespec& rhs);

// Global Vars

map<string,string> conf;	//	Configuration map: conf[key] returns value

int main(int argc, char ** argv){
	int newfiles;
	timespec RCTIME_0;	//	previous snapshot rctime
	timespec RCTIME_1;
	string SNAP_DIR;
	vector<string> syncq;	//	queue of directories to sync
	//	Load config into string-string map	
	conf = LoadConfig(CONF_DIR);
	
	cout << endl;
	Log("Started.", 0);
	cout << endl;
	
	while(1){
		RCTIME_0 = readlast_rctime();
		
		//	delete previous snapshot		
		deletesnap(RCTIME_0);
		
		Log("watching: " + conf["SND_SYNC_DIR"], 1);
		
		//	check for change
		if(checkrootforchange(conf["SND_SYNC_DIR"], RCTIME_1)){	//	RCTIME_1 > last_rctime
			Log("RCTIME_1: " + to_string(RCTIME_1.tv_sec) + "." + to_string(RCTIME_1.tv_nsec),2);
			Log("CHANGE DETECTED IN " + conf["SND_SYNC_DIR"], 0);
			
			writelast_rctime(RCTIME_1);
			this_thread::sleep_for(chrono::milliseconds(stoi(conf["RCTIME_PROP_DELAY"])));	// allow rctime to trickle to root before snapshot
			SNAP_DIR = takesnap(RCTIME_1);
			
			//	Recursively search snap of fs for changes and push files to queue if there are files to sync
			traversedir(SNAP_DIR, RCTIME_0, syncq);
			
			if(!syncq.empty()){
				cout << endl;
				Log("	Files to sync: ",1);
				for(string i : syncq){
					if(stoi(conf["LOG_LEVEL"]) >= 1)
						cout << i << endl;
				}
				if(stoi(conf["LOG_LEVEL"]) >= 1)
					cout << endl;
				newfiles = rsync(SNAP_DIR, syncq);
				cout << newfiles << " files synced.\n";
			}
		}
		
		cout << endl;
		
		sleep(stoi(conf["SYNC_FREQ"]));
	}
}

//	Function Definitions

map<string,string> LoadConfig(string path){
	//	Loads configuration file into string-string map for easy access of configuration
    ifstream input(path + "cephfssyncd.conf");
    
	if(!input){
		cout << "Unable to open configuration file. Creating " << path << "cephfssyncd.conf" << endl;
		mkdir(path.c_str(),0777);
		ofstream f;
		f.open(path + "cephfssyncd.conf");
		f <<	"SND_SYNC_DIR=\n"
				"RECV_SYNC_HOST=\n"
				"RECV_SYNC_DIR=\n"
				"LAST_RCTIME_DIR=/var/lib/ceph/cephfssync/\n"
				"SYNC_FREQ=10\n"
				"IGNORE_HIDDEN=false\n"
				"RCTIME_PROP_DELAY=5000\n"
				"COMPRESSION=false\n"
				"LOG_LEVEL=1\n"
				"# 0 = minimum logging\n"
				"# 1 = basic logging\n"
				"# 2 = debug logging\n"
				"# sync frequency in seconds\n"
				"# propagation delay in milliseconds\n"
				"# Propagation delay is to account for the limit that Ceph can\n"
				"# propagate the modification time of a file all the way back to\n"
				"# the root of the sync directory.\n"
				"# Only use compression if your network connection to your\n"
				"# backup server is slow.\n";
		f.close();
		input.open(path + "cephfssyncd.conf");
	}
	
    map<string,string> ans;
    
	string key;
    string value;
    
    while(input){
        getline(input, key, '=');
        getline(input, value, '\n');
        
        if(key[0] == '#')
			continue;				// ignore comments
			
        ans[key] = value;
    }
    input.close(); //Close the file stream
    
    //	check if attributes exist/are defined
    string attrs[] = {"SND_SYNC_DIR","RECV_SYNC_HOST","RECV_SYNC_DIR","LAST_RCTIME_DIR","SYNC_FREQ","IGNORE_HIDDEN","RCTIME_PROP_DELAY","COMPRESSION","LOG_LEVEL"};
    vector<string> errors;
    for(string i : attrs){
		try{
			if(ans.at(i) == "")
				throw 2;
		} catch (int param) {
			errors.push_back("Error: " + i + " not defined in config.");
		} catch (...) {
			errors.push_back("Error: " + i + " missing from config.");
		}
		string logstr = i + "=" + ans.at(i);
		cout << logstr << endl;
    }
    if(!errors.empty()){
		cout << endl;
		for(string i : errors){
			cout << i << endl;
		}
		cout << "Please fix these errors in the config file: " + path + "cephfssyncd.conf" << endl;
		exit(1);
	}
	string dirs[] = {"SND_SYNC_DIR","RECV_SYNC_DIR","LAST_RCTIME_DIR"};
	for(string i : dirs){
		if(ans[i].back() != '/')
			ans[i].push_back('/');		//	ensure every dir has trailing '/'
	}
    return ans; //And return the result
}

void Log(string message, int lvl){
	if(stoi(conf["LOG_LEVEL"]) >= lvl){
		cout << message << endl;
	}
}

timespec getrctime(const string & path){
	//	Returns recursive ctime of <path> or modification time of file in seconds, nanoseconds
	timespec rctime;
	if(is_dir(path.c_str())){
		char value[XATTR_SIZE];
		try{
			int strlen = getxattr(path.c_str(), "ceph.dir.rctime", value, XATTR_SIZE);
			if(strlen == -1)
				throw;
		} catch (...) {
			cerr << "Error retrieving recursive ctime (ceph.dir.rctime) of " + path;
			exit(1);
		}
		string valuestr(value);
		rctime.tv_sec = stol(valuestr.substr(0,valuestr.find(".")));
		string nanostr = valuestr.substr(valuestr.find(".")+3);		//	rctime nanoseconds: .09xxxxxxxxx, 09 must be truncated
		rctime.tv_nsec = stol(nanostr);
		
		Log("rctime of " + path + " is " + to_string(rctime.tv_sec) + "." + to_string(rctime.tv_nsec), 2);
	}else{
		struct stat t_stat;
		try{
			int strlen = stat(path.c_str(), &t_stat);
			if(strlen == -1)
				throw;
		} catch (...) {
			cerr << "Error retrieving mtime of " + path;
			exit (1);
		}
		rctime.tv_sec = t_stat.st_mtim.tv_sec;
		rctime.tv_nsec = t_stat.st_mtim.tv_nsec;
		Log("rctime of " + path + " is " + to_string(rctime.tv_sec) + "." + to_string(rctime.tv_nsec), 2);
	}
	return rctime;
}

int getsubdirs(const string & path){
	//	Returns number of subdirs in <path>
	char value[XATTR_SIZE];
	int strlen;
	try{
		strlen = getxattr(path.c_str(), "ceph.dir.subdirs", value, XATTR_SIZE);
		if(strlen == -1)
			throw;
	} catch (...) {
		cerr << "Error retrieving number of subdirs (ceph.dir.subdirs) in " + path;
		exit(1);
	}
	string valuestr(value);
	valuestr = valuestr.substr(0,strlen);
	Log("ceph.dir.subdir (number of subdirs) of " + path + " is " + valuestr,2);
	return stoi(valuestr);
}

int getfiles(const string & path){
	//	Returns number of files in <path>
	char value[XATTR_SIZE];
	int strlen;
	try{
		strlen = getxattr(path.c_str(), "ceph.dir.files", value, XATTR_SIZE);
		if(strlen == -1)
			throw;
	} catch (...) {
		cerr << "Error retrieving number of files (ceph.dir.files) in " + path;
		exit(1);
	}
	string valuestr(value);
	valuestr = valuestr.substr(0,strlen);
	Log("ceph.dir.files (number of files) of " + path + " is " + valuestr, 2);
	return stoi(valuestr);
}

string takesnap(const timespec & rctime){
	//	Create snapshot and append timestamp to name
	//	Returns snapshot of directory to sync from
	string dir = conf["SND_SYNC_DIR"] + ".snap/" + SNAP_NAME + to_string(rctime.tv_sec) + "." + to_string(rctime.tv_nsec) + "/";
	try{
		if(mkdir(dir.c_str(), 0777) == -1)
			throw;
	} catch (...) {
		cerr << "Error creating snapshot directory \"" + dir + "\".";
		exit(1);
	}
	Log("Snap dir: " + dir,2);
	return dir;
}

void traversedir(const string & path, const timespec & rctime_0, vector<string> & syncq){
	//	Recursively dives into directory. Reads files, dirs one at a time.
	//	Checks if updated since last snap then:
	//	>If dir AND isn't empty, fn is recursively called with subdir as path. 
	//	>If file AND not already queued, file is pushed onto queue awaiting sync.
	
	//	Load dir's files into queue
	Log("\nEntering " + path + "\n",2);
	vector<string> queue;
	read_directory(path, queue);
	while(!queue.empty()){
		string temp = queue.front();
		queue.erase(queue.begin());
		timespec rctime = getrctime(temp);
		if(rctime > rctime_0){
			if(is_dir(temp.c_str())){
				if(getsubdirs(temp) > 0 || getfiles(temp) > 0)	// if temp is not empty, dive in
					traversedir(temp + "/", rctime_0, syncq);
			}else{
				if(find(syncq.begin(),syncq.end(),temp) == syncq.end())	//	if temp not already queued
					syncq.push_back(temp);
			}
		}
	}
	return;
}

void read_directory(const string & path, vector<string> & v){
	//	pushes top-level contents of passed directory into queue,
	//	returns queue
    if(is_dir(path.c_str())){
		DIR * dirp = opendir(path.c_str());
		struct dirent * dp;
		while ((dp = readdir(dirp)) != NULL) {
			if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") || ((conf["IGNORE_HIDDEN"] == "true") && dp->d_name[0] == '.'))		//	ignore /.. and /., ignore hidden if set
				continue;
			v.push_back(path + dp->d_name);
		}
		
		closedir(dirp);
	}else{
		cerr << "Error openening directory " << path << ". Does it exist?" << endl;
		exit(1);
	}
}

bool is_dir(const char* path){
	//	returns true if path is dir, false if file
    struct stat buf;
    stat(path, &buf);
    return S_ISDIR(buf.st_mode);
}

int rsync(const string & SNAP_DIR, vector<string> & syncq){
	//	Sets commandline arguments and calls execution wrapper for rsync
	//	Returns number of files synced
	vector<string> args;
	int returnstatus;
	
	args = {"-a","--relative"};	// archive mode, --relative
	if(conf["COMPRESSION"] == "1" || conf["COMPRESSION"] == "true"){
		args.push_back("-z");
	}
	for(string i : syncq){
		args.push_back(SNAP_DIR + "./" + i.substr(SNAP_DIR.size(), i.size()));	//	"./" cuts the path to create subdirectories after point
	}
	int newfiles = syncq.size();
	syncq.clear();	//	remove all elements from sync queue
	
	args.push_back(conf["RECV_SYNC_HOST"] + ":" + conf["RECV_SYNC_DIR"]);	//	remote backup host ip and directory
	
	if(stoi(conf["LOG_LEVEL"]) >= 2){
		cout << "Executing \"rsync ";
		for(string i : args)
			cout << i << " ";
		cout << "\"" << endl;
	}
	
	//	convert vector<string> to char * const []
	vector<char *> argv;
	for(vector<string>::iterator itr = args.begin(); itr != args.end(); ++itr){
		argv.push_back(&(*itr)[0]);
	}
	//	null terminated:
	argv.push_back(NULL);
	
	while((returnstatus = exec("rsync", &argv[0], SNAP_DIR)) == 255){	//	retry if exit code is 255
		Log("Rsync failed. Is your remote backup server down? Trying again in 30 seconds.",0);
		this_thread::sleep_for(chrono::seconds(30));
	}
	
	if(returnstatus == 23){
		Log("Snapshot was deleted too quickly!",0);
		newfiles = 0;
	}
	
	return newfiles;
}

int exec(const char * programPath, char * const argv[], const string & SNAP_DIR){
	//	executes given command as child process
	//	returns exit status of child process
	pid_t pid;
	
	int status;
	
	pid_t c_pid = fork(); // Create a child process
	
	switch (c_pid) {
	case -1: // Error 
		cerr << "Error: fork() failed.\n";
		exit(1);
	case 0: // Child process
		pid = getpid();
		cout << programPath << " process created with pid " << pid << c_pid << "\n";
		execvp(programPath, argv); // Execute the program
		cerr << "rsync failed. Try reinstalling and make sure rsync is in your path.";
		exit(1);
	default: // Parent process 
		if( (pid = wait(&status)) < 0){
			cerr << "Error while waiting for child process to finish.\n";
			exit(1);
		}
		
		Log("Sync process exited with " + to_string(WEXITSTATUS(status)) + "\n", 2);
	}
	Log("Rsync exit status: " + to_string(WEXITSTATUS(status)),1);
	return WEXITSTATUS(status);
}

bool checkrootforchange(const string & path, timespec & RCTIME_1){
	//	reads rctimes/mtimes of all top-level root files and folders
	//	updates value of RCTIME_1 to highest time above last sync time
	//	returns true if any are higher than last_rctime
	vector<string> rootfiles;
	timespec RCTIME_0 = readlast_rctime();
	timespec temp;
	RCTIME_1 = RCTIME_0;
	
	//	load in top-level contents
	read_directory(path, rootfiles);
	
	for(string i : rootfiles){
		Log("Checking change in: " + i,2);
		
		temp = getrctime(i);
		Log("RCTIME of dir: " + to_string(temp.tv_sec) + "." + to_string(temp.tv_nsec),2);
		
		if(temp > RCTIME_0 && temp > RCTIME_1){
			RCTIME_1 = temp;
		}
	}
	
	Log("rctime_0: " + to_string(RCTIME_0.tv_sec) + "." + to_string(RCTIME_0.tv_nsec), 2);
	Log("rctime_1: " + to_string(RCTIME_1.tv_sec) + "." + to_string(RCTIME_1.tv_nsec), 2);
	
	return RCTIME_1 > RCTIME_0;
}

void writelast_rctime(const timespec & rctime){
	//	saves timestamp of snapshot to file
	ofstream f;
	f.open(LAST_RCTIME);
	f << rctime.tv_sec << endl << rctime.tv_nsec << endl;		//	revert back to 0 nanoseconds due to slight delay in rctime update for dirs
	f.close();
}

timespec readlast_rctime(void){
	//	Reads timestamp of last snapshot from file. If file
	//	does not exist, file is created and last_rctime is set to 0.
	Log("fn: readlast_rctime",3);
	//	read last rctime (0 if it doesn't exist)
	timespec RCTIME_0;
	ifstream last_rctime;
	last_rctime.open(LAST_RCTIME);
	if(last_rctime){
		Log("last_rctime exists, init RCTIME0=last_rctime", 2);
	}else{
		Log("last_rctime does not exist, init RCTIME0=0", 2);
		mkdir(conf["LAST_RCTIME_DIR"].c_str(), 0777);
		ofstream f;
		f.open(LAST_RCTIME);
		f << "0" << endl << "0" << endl;
		f.close();
		last_rctime.open(LAST_RCTIME);
	}
	last_rctime >> RCTIME_0.tv_sec;
	last_rctime >> RCTIME_0.tv_nsec;
	last_rctime.close();
	cout << endl << "RCTIME_0: " << RCTIME_0.tv_sec << '.' << RCTIME_0.tv_nsec << endl;
	return RCTIME_0;
}

void deletesnap(const timespec & rctime){
	//	deletes snapshot with timestamp <rctime>
	Log("Deleting previous snapshot",2);
	string path = conf["SND_SYNC_DIR"] + ".snap/" + SNAP_NAME + to_string(rctime.tv_sec) + "." + to_string(rctime.tv_nsec);
	if(is_dir(path.c_str())){
		if(rmdir(path.c_str()) == -1){
			Log("Error deleting snapshot: " + path, 0);
			Log("Delete snapshot manually.",0);
		}
	}
}

bool operator >(const timespec& lhs, const timespec& rhs){
    //return ((lhs.tv_sec > rhs.tv_sec) || ((lhs.tv_sec == rhs.tv_sec) && (lhs.tv_nsec > rhs.tv_nsec)));
    if(lhs.tv_sec == rhs.tv_sec){
		return lhs.tv_nsec > rhs.tv_nsec;
	}else{
		return lhs.tv_sec > rhs.tv_sec;
	}
}

bool operator <(const timespec& lhs, const timespec& rhs){
	if(lhs.tv_sec == rhs.tv_sec){
		return lhs.tv_nsec < rhs.tv_nsec;
	}else{
		return lhs.tv_sec < rhs.tv_sec;
	}
}

#include <iostream>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <string>
using namespace std;

enum mode {f, d};

#define XATTR_SIZE 10000

bool is_dir(const char* path){
	//	returns true if path is dir, false if file
    struct stat buf;
    stat(path, &buf);
    return S_ISDIR(buf.st_mode);
}

timespec getrctime(const string & path, mode mode_){
	//	Returns recursive ctime of <path> in d mode and modification time of file in f mode in seconds, nanoseconds
	timespec rctime;
	if(mode_ == d){
		int strlen;
		char value[XATTR_SIZE];
		try{
			strlen = getxattr(path.c_str(), "ceph.dir.rctime", value, XATTR_SIZE);
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
		
	}else if(mode_ == f){
		struct stat t_stat;
		stat(path.c_str(), &t_stat);
		rctime.tv_sec = t_stat.st_mtim.tv_sec;
		rctime.tv_nsec = t_stat.st_mtim.tv_nsec;
	}
	return rctime;
}

int main(){
	timespec time1 = getrctime("/mnt/cephfs/fsgw/daemontest/newdir/newfile",f);
	timespec time2 = getrctime ("/mnt/cephfs/fsgw/daemontest/newdir/",d);
	cout << "newdir/newfile: " << time1.tv_sec << "." << time1.tv_nsec << endl;
	cout << "newdir/:        " << time2.tv_sec << "." << time2.tv_nsec << endl;
	return 0;
}

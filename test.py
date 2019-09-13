# Ceph File System Sync Daemon Testing Script

# Created: Josh Boudreau	2019-01-17

# This script creates a directory tree with depth given in the first
# argument, width given in the second, and number of files to create
# given in the third.
# Example: python test.py 3 2 10
#
# Directory tree created:
#             1
#          1<
#         /   2
#       .1   
#      /  \   1
#     /    2<
#    /        2
# '.'
#    \        1
#     \    1<
#      \  /   2
#       '2   
#         \   1
#          2<
#             2
# Number of directories created = SUM ( width )^n, n = {1..depth}
# Files are placed randomly within the tree at a random interval between 0.1 and maxwait_ seconds.

#!/usr/bin/python

import os, sys, random, time, math
from datetime import datetime

random.seed(datetime.now())

if not len(sys.argv) == 5:
	print "Script takes four arguments: dir depth, dir width, number of files, and file size in kB."
	exit()

path = "/mnt/cephfs/test"

depth_ = int(sys.argv[1])

width_ = int(sys.argv[2])

files = int(sys.argv[3])

fsize = int(sys.argv[4])

temp_path = path

maxwait_ = 3

def calc(d, w):
	sum = 0
	for n in range(d):
		sum += math.pow(w,n+1)
	if sum > 1000:
		print str(int(sum)) + " dirs will be created. are you sure?"
		choice = raw_input("y/n:")
		if (choice == "y") or (choice == "Y") or (choice == "yes") or (choice == "Yes"):
			return
		else:
			exit()
	return

def recurse(path, depth):
	if depth <= 0:
		return
	for i in range(width_):
		if not os.path.exists(os.path.join(path,str(i+1))):
			os.mkdir(os.path.join(path,str(i+1)))
		
		recurse(os.path.join(path,str(i+1)),depth-1)

def touch(fname):
	f = open(fname,"wb")
	for i in range(fsize):
		f.write('\0' * 1024) # single kilobyte
	f.close()
	return


def recursefile(path, depth, filename):
	if depth <= 0:
		touch(os.path.join(path,str(filename) + ".img"))
		print "Made file: " + os.path.join(path,str(filename) + ".img")
		return
	dir_ = random.randint(1,width_)
	recursefile(os.path.join(path,str(dir_)),depth-1,filename)

calc(depth_,width_)

recurse(path, depth_)

for i in range(files):
	depth = random.randint(0,depth_)
	recursefile(path,depth,i+1)
	time.sleep(random.uniform(0.1,maxwait_))


print "Done"

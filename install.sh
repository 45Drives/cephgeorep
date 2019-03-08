#!/usr/bin/env bash

echo Building executable and copying to /usr/bin/ as cephfssyncd

g++ -std=c++11 cephfssyncd.cpp -o /usr/bin/cephfssyncd

echo Copying service file to /etc/systemd/system/

cp cephfssyncd.service /etc/systemd/system/

echo Done. Please configure daemon in /etc/ceph/cephfssyncd.conf

read -p "Open config file now? " -n 1 -r
echo    # (optional) move to a new line
if [[ $REPLY =~ ^[Yy]$ ]]
then
    printf "SND_SYNC_DIR=\n\
RECV_SYNC_HOST=\n\
RECV_SYNC_DIR=\n\
LAST_RCTIME_DIR=/var/lib/ceph/cephfssync/\n\
SYNC_FREQ=10\n\
IGNORE_HIDDEN=false\n\
RCTIME_PROP_DELAY=100\n\
COMPRESSION=false\n\
LOG_LEVEL=1\n\
# 0 = minimum logging\n\
# 1 = basic logging\n\
# 2 = debug logging\n\
# sync frequency in seconds\n\
# propagation delay in milliseconds\n\
# Propagation delay is to account for the limit that Ceph can\n\
# propagate the modification time of a file all the way back to\n\
# the root of the sync directory.\n\
# Only use compression if your network connection to your\n\
# backup server is slow.\n" > /etc/ceph/cephfssyncd.conf
    vi /etc/ceph/cephfssyncd.conf
fi

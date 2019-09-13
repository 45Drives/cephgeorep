#!/usr/bin/env bash

if [[ $(id -u) != 0 ]]; then
    echo "Script must be run as root."
    exit 1
fi

read -p "Install cephgeorep daemon? This will overwrite /etc/ceph/cephfssyncd.conf. [Y/n] " -r
if [[ $REPLY =~ ^[Nn]$ ]] || [[ $REPLY =~ ^[Nn][Oo]$ ]]; then
    echo "Exiting."
    exit 0
fi

echo "Building executable and copying to /usr/bin/ as cephfssyncd"

make
if [[ $? -ne 0 ]]; then
    echo "Build failed. Make sure package 'gcc-c++' and boost libs are installed and retry."
    exit 1
fi

echo "Copying service file to /etc/systemd/system/"

cp cephfssyncd.service /etc/systemd/system/

# initializing daemon config
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

read -p "Configure daemon now? [Y/n] " -r
if [[ ! $REPLY =~ ^[Nn]$ ]] && [[ ! $REPLY =~ ^[Nn][Oo]$ ]]; then
    vi /etc/ceph/cephfssyncd.conf
    echo "Start cephgeorep service with \"systemctl start cephfssyncd\"."
else
    echo "Please configure daemon in /etc/ceph/cephfssyncd.conf"
    echo "Start cephgeorep service with \"systemctl start cephfssyncd\"."
fi
read -p "Enable daemon service now to start at boot? [Y/n] " -r
if [[ ! $REPLY =~ ^[Nn]$ ]] && [[ ! $REPLY =~ ^[Nn][Oo]$ ]]; then
    systemctl enable cephfssyncd
    echo "Service enabled."
else
    echo "Enable service manually with \"systemctl enable cephfssyncd\"."
fi
echo "Installation finished."
exit 0

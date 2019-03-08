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
    nano /etc/ceph/cephfssyncd.conf
fi

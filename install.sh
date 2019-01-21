#!/usr/bin/env bash

g++ -std=c++11 cephfssyncd.cpp -o /usr/bin/cephfssyncd

cp cephfssyncd.service /etc/systemd/system/

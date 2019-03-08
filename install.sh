#!/usr/bin/env bash

g++ -std=c++11 cephfssyncd.cpp -o /usr/bin/cephgeorepd

cp cephgeorepd.service /etc/systemd/system/

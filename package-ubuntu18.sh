#!/usr/bin/env bash

#    Copyright (C) 2019-2021 Joshua Boudreau <jboudreau@45drives.com>
# 
#    This file is part of cephgeorep.
# 
#    cephgeorep is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 2 of the License, or
#    (at your option) any later version.
# 
#    cephgeorep is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.

if [[ "$#" == 1 && "$1" == "clean" ]]; then
	pushd ubuntu18
	rm -f cephgeorep.postrm.debhelper cephgeorep.substvars debhelper-build-stamp files
	rm -rf .debhelper cephgeorep
	popd
	rm -rf dist/ubuntu18
	exit 0
fi

command -v docker > /dev/null 2>&1 || {
	echo "Please install docker.";
	exit 1;
}

# if docker image DNE, build it
if [[ "$(docker images -q cephgeorep-ubuntu18-builder 2> /dev/null)" == "" ]]; then
	docker build -t cephgeorep-ubuntu18-builder - < docker/ubuntu18
	res=$?
	if [ $res -ne 0 ]; then
		echo "Building docker image failed."
		exit $res
	fi
fi

make clean

mkdir -p dist/ubuntu18

# mirror current directory to working directory in container, and mirror dist/ubuntu to .. for deb output
docker run -u $(id -u):$(id -g) -w /home/deb/build -it -v$(pwd):/home/deb/build -v$(pwd)/ubuntu18:/home/deb/build/debian -v$(pwd)/dist/ubuntu18:/home/deb --rm cephgeorep-ubuntu18-builder dpkg-buildpackage -us -uc -b
res=$?
if [ $res -ne 0 ]; then
	echo "Packaging failed."
	exit $res
fi

rmdir dist/ubuntu18/build debian

echo "deb is in dist/ubuntu18/"

exit 0

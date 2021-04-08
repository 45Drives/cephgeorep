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

TMP_DIR=/tmp/cephgeorep

if [[ "$#" == 1 && "$1" == "clean" ]]; then
	rm -rf $TMP_DIR
	rm -rf dist/el7
	exit 0
fi

command -v docker > /dev/null 2>&1 || {
	echo "Please install docker.";
	exit 1;
}

# if docker image DNE, build it
if [[ "$(docker images -q cephgeorep-el7-builder 2> /dev/null)" == "" ]]; then
	docker build -t cephgeorep-el7-builder - < docker/el7
	res=$?
	if [ $res -ne 0 ]; then
		echo "Building docker image failed."
		exit $res
	fi
fi

make clean

mkdir -p dist/el7
mkdir -p $TMP_DIR

SOURCE_NAME=cephgeorep-$(grep Version el7/cephgeorep.spec --color=never | awk '{print $2}')
SOURCE_PATH=$TMP_DIR/$SOURCE_NAME
mkdir -p $SOURCE_PATH
SOURCE_LOC="$(dirname "$SOURCE_PATH")"

# build statically in ubuntu first
./docker-make -j8 static
res=$?
if [ $res -ne 0 ]; then
	echo "Building failed."
	exit $res
fi

make DESTDIR=$SOURCE_PATH PACKAGING=1 install

pushd $SOURCE_LOC
tar -czvf $SOURCE_NAME.tar.gz $SOURCE_NAME
popd

# build rpm from source tar and place it dist/el7 by mirroring dist/el7 to rpmbuild/RPMS
docker run -u $(id -u):$(id -g) -w /home/rpm/rpmbuild -it -v$SOURCE_LOC:/home/rpm/rpmbuild/SOURCES -v$(pwd)/dist/el7:/home/rpm/rpmbuild/RPMS -v$(pwd)/el7:/home/rpm/rpmbuild/SPECS --rm cephgeorep-el7-builder rpmbuild -ba SPECS/cephgeorep.spec
res=$?
if [ $res -ne 0 ]; then
	echo "Packaging failed."
	exit $res
fi

rm -rf $SOURCE_LOC

echo "rpm is in dist/el7/"

exit 0

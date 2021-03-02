#!/usr/bin/env bash

if [[ "$#" == 1 && "$1" == "clean" ]]; then
	pushd debian
	rm -f cephgeorep.postrm.debhelper cephgeorep.substvars debhelper-build-stamp files
	rm -rf .debhelper cephgeorep
	popd
	exit 0
fi

command -v docker > /dev/null 2>&1 || {
	echo "please install docker.";
	exit 1;
}

if [[ "$(docker images -q ubuntu-builder 2> /dev/null)" == "" ]]; then
	docker build -t ubuntu-builder - < docker/ubuntu
	res=$?
	if [ $res -ne 0 ]; then
		echo "building docker image failed."
		exit $res
	fi
fi

make clean

mkdir -p dist/ubuntu

docker run -u $(id -u):$(id -g) -w /home/deb/build -it -v$(pwd):/home/deb/build -v$(pwd)/dist/ubuntu:/home/deb --rm ubuntu-builder dpkg-buildpackage -us -uc -b
res=$?
if [ $res -ne 0 ]; then
	echo "packaging failed."
	exit $res
fi

rmdir dist/ubuntu/build

echo "deb is in dist/ubuntu/"

exit 0

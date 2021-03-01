#!/usr/bin/env bash

command -v docker > /dev/null 2>&1 || {
	echo "please install docker.";
	exit 1;
}

if [[ "$(docker images -q ubuntu-builder 2> /dev/null)" == "" ]]; then
	docker build -t ubuntu-builder .
	res=$?
	if [ $res -ne 0 ]; then
		echo "building docker image failed."
		exit $res
	fi
fi

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

#!/usr/bin/env bash

command -v docker > /dev/null 2>&1 || {
	echo "please install docker.";
	exit 1;
}

if [[ "$(docker images -q ubuntu-builder 2> /dev/null)" == "" ]]; then
	docker build -t ubuntu-builder .
fi

mkdir -p dist/ubuntu

docker run -w /home/deb/build -it -v$(pwd):/home/deb/build -v$(pwd)/dist/ubuntu:/home/deb --rm ubuntu-builder dpkg-buildpackage -us -uc -b

echo "deb is in dist/ubuntu/"

#!/usr/bin/env bash

command -v docker > /dev/null 2>&1 || {
	echo "please install docker.";
	exit 1;
}

if [[ "$(docker images -q el7-builder 2> /dev/null)" == "" ]]; then
	docker build -t el7-builder - < docker/centos7
	res=$?
	if [ $res -ne 0 ]; then
		echo "building docker image failed."
		exit $res
	fi
fi

make clean

mkdir -p dist/{el7,tmp}

SOURCE_DIR=cephgeorep-$(grep Version centos7/cephgeorep.spec --color=never | awk '{print $2}')
DEST=dist/tmp/$SOURCE_DIR
mkdir -p $DEST

# build statically in ubuntu first
./docker-make -j8 static
res=$?
if [ $res -ne 0 ]; then
	echo "building failed."
	exit $res
fi

make DESTDIR=$DEST PACKAGING=1 install

pushd $DEST/..
tar -czvf $SOURCE_DIR.tar.gz $SOURCE_DIR
popd

docker run -u $(id -u):$(id -g) -w /home/rpm/rpmbuild -it -v$(pwd)/dist/tmp:/home/rpm/rpmbuild/SOURCES -v$(pwd)/dist/el7:/home/rpm/rpmbuild/RPMS -v$(pwd)/centos7:/home/rpm/rpmbuild/SPECS --rm el7-builder rpmbuild -ba SPECS/cephgeorep.spec
res=$?
if [ $res -ne 0 ]; then
	echo "packaging failed."
	exit $res
fi

rm -rf dist/tmp

echo "rpm is in dist/centos7/"

exit 0

FROM ubuntu

ENV TZ=America/Glace_Bay
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

LABEL description="Container in which to build ubuntu applications"

RUN apt update && apt install -y g++ rsync zip openssh-server make libboost-all-dev librocksdb-dev build-essential fakeroot devscripts debhelper libtbb-dev

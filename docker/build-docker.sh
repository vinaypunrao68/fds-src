#!/bin/sh
if [! -x `which docker` ] ; then
	echo "Cannot find the docker executable - please install following the docs here:"
	echo "http://docs.docker.com/installation/"
fi
echo "Building on xwing"
export DOCKER_HOST=tcp://xwing.formationds.com:2375
docker build -t spoon-fds spoon-fds

echo "Building on bld-dev-01"
export DOCKER_HOST=tcp://10.2.10.20:2375
docker build -t spoon-fds spoon-fds

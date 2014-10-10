#!/bin/sh
if [ ! -x `which docker` ] ; then
	echo "Cannot find the docker executable - please install following the docs here:"
	echo "http://docs.docker.com/installation/"
fi

IMG_NAME="coke.formationds.com:5000/fds_dev"
echo "Building on xwing"
export DOCKER_HOST=tcp://xwing.formationds.com:2375
docker build -t $IMG_NAME dev_image
docker push $IMG_NAME

export DOCKER_HOST=tcp://10.2.10.20:2375
docker pull $IMG_NAME

export DOCKER_HOST=tcp://10.1.10.102:2375
docker pull $IMG_NAME

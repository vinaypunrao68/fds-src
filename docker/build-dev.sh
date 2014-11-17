#!/bin/sh
if [ ! -x `which docker` ] ; then
	echo "Cannot find the docker executable - please install following the docs here:"
	echo "http://docs.docker.com/installation/"
fi

IMG_NAME="registry.formationds.com:5000/fds_dev"

echo "Building on xwing"
export DOCKER_HOST=tcp://xwing.formationds.com:2375
docker build --no-cache -t $IMG_NAME dev_image
docker push $IMG_NAME

export DOCKER_HOST=tcp://bld-dev-01.formationds.com:2375
docker pull $IMG_NAME

export DOCKER_HOST=tcp://bld-dev-02.formationds.com:2375
docker pull $IMG_NAME

export DOCKER_HOST=tcp://tie-fighter.formationds.com:2375
docker pull $IMG_NAME

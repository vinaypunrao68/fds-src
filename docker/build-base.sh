#!/bin/bash -e
echo "THIS SCRIPT IS DEPRECATED - please use docker_build.py --type base"
exit 1

if [ ! -x `which docker` ] ; then
  echo "Cannot find the docker executable - please install following the docs here:"
  echo "http://docs.docker.com/installation/"
fi

IMG_NAME="registry.formationds.com:5000/fds_base"
BUILD_URL="tcp://fre-build-04.formationds.com:2375"
PULL_HOSTS=( "fre-build-01" "fre-build-02" "fre-build-03" "fre-build-04" "bld-dev-01" "bld-dev-02" )

echo "Building on fre-build-04"
docker --host ${BUILD_URL} build --no-cache -t $IMG_NAME base_image
docker --host ${BUILD_URL} push $IMG_NAME

for HOST in ${PULL_HOSTS}
do
  echo "Pull image to ${HOST}"
  docker --host tcp://${HOST}.formationds.com:2375 pull $IMG_NAME
done

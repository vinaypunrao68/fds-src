Docker Images
=============

This directory contains a repository of docker images used for various
development tasks. Currently there are 2 docker images here:

## fds_base
This is a base image which is used to build the fds_dev image. The build
process for this image is long & contains stuff that shouldn't change
often so if you are iterating on fds_dev you don't have to rebuild these
portions each time. 

This is built by running ./build-base.sh

## fds_dev
This image is a derivative of fds_base and is used for all dev
environments inside Docker (spoon & Jenkins slaves).

This is built by running ./build-dev.sh

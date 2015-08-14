#!/bin/bash

workspace=/home/pairing/projects/fds-src
pushd $workspace/ansible
./scripts/teardown_ec2_cluster.sh matteo_ec2
popd

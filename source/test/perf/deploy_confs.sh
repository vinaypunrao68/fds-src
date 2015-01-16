#!/bin/bash

tag=$1
host=$2

sed s/HOST/$host/ < regress/config/ansible_hosts.$tag > ../../../ansible/inventory/$host
pushd ../../..
./ansible/scripts/deploy_config_files_only.sh -s $host
popd
cp /root/fds_configs/* ../../config/etc/
cp /root/fds_configs/formation.conf ../../test/


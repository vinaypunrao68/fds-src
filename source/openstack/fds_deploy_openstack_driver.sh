#!/bin/bash

USER=root
PASSWD=passwd
for node in controller block1 compute1 compute2; do
    echo "Deploying openstack driver to node: $node"
    sshpass -p $PASSWD ssh $USER@$node mkdir -p /$USER/openstack_package
    sshpass -p $PASSWD scp openstack_package.tar.gz $USER@$node:/root/openstack_package
    sshpass -p $PASSWD ssh $USER@$node tar xzf /$USER/openstack_package/openstack_package.tar.gz
done;

for node in controller; do
    sshpass -p $PASSWD ssh $USER@$node "cd /$USER/openstack_package/controller; ./fds_install_ctrl.sh"
done;


for node in block1; do
    sshpass -p $PASSWD ssh $USER@$node "cd /$USER/openstack_package/block; ./fds_install_block.sh"
done;

for node in compute1 compute2; do
    sshpass -p $PASSWD ssh $USER@$node "cd /$USER/openstack_package/compute; ./fds_install_compute.sh"
done;

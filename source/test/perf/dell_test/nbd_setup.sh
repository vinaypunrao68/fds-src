#!/bin/bash
ssh perf1-node1 '/fds/bin/fdscli --volume-create blockVolume -i 1 -s 10240 -p 50 -y blk'
sleep 10
ssh perf1-node1 '/fds/bin/fdscli --volume-modify blockVolume -s 10240 -g 0 -m 0 -r 10'
sleep 10
disk=`../../../cinder/nbdadm.py attach perf1-node1 blockVolume`
echo $disk > .disk


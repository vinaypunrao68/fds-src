#!/bin/bash
ssh perf1-node1 'pushd /fds/sbin && ./fdsconsole.py volume create blockVolume --vol-type block --blk-dev-size 10485760 && popd'
sleep 10
ssh perf1-node1 'pushd /fds/sbin && ./fdsconsole.py volume modify blockVolume --minimum 0 --maximum 0 --priority 1 && popd'
sleep 10
disk=`../../../cinder/nbdadm.py attach perf1-node1 blockVolume`
echo $disk > .disk


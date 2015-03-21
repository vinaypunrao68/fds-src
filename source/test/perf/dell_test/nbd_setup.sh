#!/bin/bash
node=$1
echo "Setting up nbd on $node"
ssh $node 'bash -c "pushd /fds/sbin && ./fdsconsole.py accesslevel debug && ./fdsconsole.py volume create blockVolume --vol-type block --blk-dev-size 10485760 && popd"'
sleep 10
ssh $node 'bash -c "pushd /fds/sbin && ./fdsconsole.py volume modify blockVolume --minimum 0 --maximum 0 --priority 1 && popd"'
sleep 10
disk=`../../../cinder/nbdadm.py attach $node blockVolume`
echo $disk > .disk


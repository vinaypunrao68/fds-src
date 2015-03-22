#!/bin/bash
node=$1
echo "Setting up s3 on $node"
pushd /fds/sbin && ./fdsconsole.py accesslevel debug && ./fdsconsole.py volume create volume0 --vol-type object && popd
sleep 10

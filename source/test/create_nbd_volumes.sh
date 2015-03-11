#!/bin/bash

for i in 1 2 3 4
do
    pushd ../tools
    ./fdsconsole.py volume create  Volume$i --vol-type block --blk-dev-size 10485760
    sleep 1
    ./fdsconsole.py volume modify Volume$i  --minimum 0 --maximum 10000 --priority 1
    sleep 1
    ../cinder/nbdadm.py attach localhost Volume$i 
    sleep 1
    popd
done


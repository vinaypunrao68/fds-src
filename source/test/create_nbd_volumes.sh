#!/bin/bash

for i in 1 2 3 4
do
    /fds/bin/fdscli --volume-create Volume$i -i 1 -s 10240 -p 50 -y blk
    sleep 1
    /fds/bin/fdscli --volume-modify Volume$i -s 10240 -g 0 -m 10000 -r 1
    sleep 1
    let j=$i-1
    nbd-client -N Volume1 10.1.10.102 /dev/nbd$j -b 4096
    sleep 1
done


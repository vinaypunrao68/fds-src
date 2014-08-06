#!/bin/bash

MAX_NVOLS=$1

pkill iostat
pkill collectl
rm -f /tmp/fdstr*
../tools/fds stop
sleep 1
../tools/fds clean
sleep 1
../tools/fds start
sleep 10
for i in `seq 0 $MAX_NVOLS` 
do
    curl -v -X PUT http://localhost:8000/volume$i
    sleep 1
done

#!/bin/bash

pkill iostat
pkill collectl
rm -f /tmp/fdstr*
../tools/fds stop
sleep 1
../tools/fds clean
sleep 1
../tools/fds start
sleep 10
curl -v -X PUT http://localhost:8000/volume

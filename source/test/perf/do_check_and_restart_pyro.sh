#!/bin/bash

check=`bash pyroadm.sh -c | grep all | awk '{print $2}'`

echo "is Pyro4 up? $check"

if [ $check -eq 0 ]; then
     echo "starting pyro4"
     ./pyroadm.sh -d
     while [  $check -eq 0 ]; do
         ./pyroadm.sh -s
         sleep 1
         check=`bash pyroadm.sh -c | grep all | awk '{print $2}'`
     done 
fi


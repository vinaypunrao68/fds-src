#!/bin/bash

check=`python pyroadm.py -c | grep all | awk '{print $2}'`

echo "is Pyro4 up? $check"

if [ $check -eq 0 ]; then
     echo "starting pyro4"
     python pyroadm.py -d
     while [  $check -eq 0 ]; do
         python pyroadm.py -s
         sleep 1
         check=`python pyroadm.py -c | grep all | awk '{print $2}'`
     done 
fi


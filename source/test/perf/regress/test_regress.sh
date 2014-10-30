#!/bin/bash

i=0
while [ 1 -eq 1 ] 
do
    let i=$i+1
    ./perf_regr2.sh 2>%1 | tee log-$i
    sleep 60
done

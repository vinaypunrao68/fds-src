#!/bin/bash
for f in `ls -d $1` ; do
    iops=`grep iops $f | sed -e 's/[ ,=:]/ /g' | awk '{e+=$7}END{print e}'`
    frequency=`echo $f | awk  -F'[./]' '{print $2}'`
    policy=`echo $f | awk  -F'[./]' '{print $4}'`
    latency=`grep clat $f | grep avg| awk -F '[,=:()]' '{print $9}' | awk '{i+=1; e+=$1}END{print e/i}'`

    echo policy=$policy frequency=$frequency iops=$iops latency=$latency
    echo file=$f > .data
    echo iops=$iops >> .data
    echo frequency=$frequency >> .data
    echo policy=$policy >> .data
    echo latency=$latency >> .data
    ../common/push_to_influxdb.py tiering_test .data
done



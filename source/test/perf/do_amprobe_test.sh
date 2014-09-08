#!/bin/bash

results=$1
if [ ! -d $results ]
then
    mkdir $results
fi

function test1() {
    rm -fr /tmp/fdstrgen*
    local size=$1
    local res=$2
    local cache=$3
    local outs=$4
    echo "test1: $size $res $cache $outs"
    ./test/fds-tool.py -f fdstool.cfg -d -c
    sleep 1
    sed -e "s/default_max_entries = .*/default_max_entries = $cache/" config/etc/platform.conf > platform1.conf
    sed -e "s/probe_outstanding_reqs = .*/probe_outstanding_reqs = $outs/" platform1.conf > platform.conf
    scp platform.conf han:/fds/etc/
    sleep 1
    ./test/fds-tool.py -f fdstool.cfg -u -v
    sleep 1
    curl -v -X PUT http://han:8000/volume0
    sleep 5
    ./test/traffic_gen.py -t 1 -n 10000 -T PUT -s $size -F 1000 -v 1 -u -N han
    sleep 5
    python test/perf/gen_json.py -n 100000 -s $size > am-get.json
    curl -v -X PUT -T am-get.json http://han:8080
    sleep 1
    ssh han 'cat /fds/var/logs/am.*' |grep CRITICAL > $res/am-probe-$size-$cache-$outs-read.out
}

caches="0 500 1200"
outstandings="25 50 100 200 300 400 500"

if [ -d counters.dat ]
then
    rm -f counters.dat
fi

python ./test/perf/counter_server.py &
cntr_pid=$!

echo "counter server on $cntr_pid"
sleep 5
for outs in $outstandings
do
    for cache in $caches
    do    
        #sizes="4096 65536 133120"
        sizes="4096 65536"
        for size in $sizes
        do
            test1 $size $results $cache $outs
            sleep 10
        done
        
        #sizes="262144 524288 786432 1048576 1310720 1572864 1835008 2097152"
        sizes="262144 524288 1048576"
        #sizes="524288 786432 1048576 2097152"
        for size in $sizes
        do
            test1 $size $results $cache $outs
            sleep 10
        done
    done
done
kill -9 $cntr_pid

#!/bin/bash

outdir=$1
size="16m"
bsizes="4096"
iodepths="50"
workers="8"
workloads="randread"

#bsizes="4096"
#iodepths="1 2 4 8 16 32 64 128 256"
#iodepths="32"
#workers="4"
#workloads="randread"

function process_results {
    local f=$1
    local numjobs=$2
    local workload=$3
    local bs=$4
    local iodepth=$5
    local disksize=$6

    iops=`grep iops $f | sed -e 's/[ ,=:]/ /g' | awk '{e+=$7}END{print e}'`
    latency=`grep clat $f | grep avg| awk -F '[,=:()]' '{print $9}' | awk '{i+=1; e+=$1}END{print e/i}'`

    echo file=$f > .data
    echo numjobs=$numjobs >> .data
    echo workload=$workload >> .data
    echo bs=$bs >> .data
    echo iodepth=$iodepth >> .data
    echo disksize=$disksize >> .data
    echo iops=$iops >> .data
    echo latency=$latency >> .data
    ../common/push_to_influxdb.py dell_test .data
}

if [ ! -e .disk ] ; then
    echo cannot find nbd disk definition file
    exit 1
fi
nbd_disk=`cat .disk`
fio --name=write --rw=write --filename=$nbd_disk --bs=4096 --numjobs=1 --iodepth=32 --ioengine=libaio --direct=1 --size=$size

for worker in $workers ; do
    for workload in $workloads ; do
        for bs in $bsizes ; do
            for d in $iodepths ; do
            #sync
            #echo 3 > /proc/sys/vm/drop_caches
            fio --name=test --rw=$workload --filename=$nbd_disk --bs=$bs --numjobs=$worker --iodepth=$d --ioengine=libaio --direct=1 --size=$size --time_based --runtime=60 | tee $outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size
            process_results $outfile $worker $workload $bs $d $size
            done
        done
    done
done

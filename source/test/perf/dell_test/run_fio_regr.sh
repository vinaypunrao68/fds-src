#!/bin/bash

outdir=$1
size="16m"
bsizes="4096"
iodepths="128"
workers="8"
workloads="randread"

#bsizes="4096"
#iodepths="1 2 4 8 16 32 64 128 256"
#iodepths="32"
#workers="4"
#workloads="randread"

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
            done
        done
    done
done

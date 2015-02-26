#!/bin/bash

outdir=$1
size=8g
bsizes="512 4096 8192 65536 524288"
iodepths="1 2 4 8 16 32 64 128 256"
workers="4"
workloads="randread read randwrite write"

#bsizes="4096"
#iodepths="1 2 4 8 16 32 64 128 256"
#workers="4"
#workloads="randread"
fio --name=write --rw=write --filename=/dev/nbd15 --bs=4096 --numjobs=1 --iodepth=32 --ioengine=libaio --direct=1 --size=$size

for worker in $workers ; do
    for workload in $workloads ; do
        for bs in $bsizes ; do
            for d in $iodepths ; do
            #sync
            #echo 3 > /proc/sys/vm/drop_caches
            fio --name=test --rw=$workload --filename=/dev/nbd15 --bs=$bs --numjobs=$worker --iodepth=$d --ioengine=libaio --direct=1 --size=$size --time_based --runtime=60 | tee $outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size
            done
        done
    done
done

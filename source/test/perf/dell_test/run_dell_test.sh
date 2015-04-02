#!/bin/bash

#########################

function volume_setup {
    local max_obj_size=$1
    local node=$2
    local vol=$3
    ssh $node "'pushd /fds/sbin && ./fdsconsole.py accesslevel admin && ./fdsconsole.py volume create $vol --vol-type block --blk-dev-size 10485760 --max-obj-size $max_obj_size && popd'"
    sleep 10
    ssh $node 'pushd /fds/sbin && ./fdsconsole.py volume modify $vol --minimum 0 --maximum 0 --priority 1 && popd'
    sleep 10
    disk=`../../../cinder/nbdadm.py attach $node $vol`
    echo $disk
}

function volume_detach {
    local vol=$1
    ../../../cinder/nbdadm.py detach $vol
}

function process_results {
    #outfile=$outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size
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

#########################

outdir=$1
node=$2

size=8g
bsizes="512 4096 8192 65536 524288"
iodepths="1 2 4 8 16 32 64 128 256"
workers="4"
workloads="randread read randwrite write"

for bs in $bsizes ; do
    nbd_disk=`volume_setup $bs $node volume_$bs`
    fio --name=write --rw=write --filename=$nbd_disk --bs=$bs --numjobs=1 --iodepth=32 --ioengine=libaio --direct=1 --size=$size
    for worker in $workers ; do
        for workload in $workloads ; do
                for d in $iodepths ; do
                #sync
                #echo 3 > /proc/sys/vm/drop_caches
                outfile=$outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size
                fio --name=test --rw=$workload --filename=$nbd_disk --bs=$bs --numjobs=$worker --iodepth=$d --ioengine=libaio --direct=1 --size=$size --time_based --runtime=60 | tee $outfile
                process_results $outfile
                done
            done
        done
    volume_detach $vol
done

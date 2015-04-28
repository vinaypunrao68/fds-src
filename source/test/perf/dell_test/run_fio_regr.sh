#!/bin/bash

#####################

function volume_setup {
    local max_obj_size=$1
    local node=$2
    local vol=$3
    pushd /fds/sbin 
    ./fdsconsole.py accesslevel admin 
    ./fdsconsole.py volume create $vol --vol-type block --blk-dev-size 10485760 --max-obj-size $max_obj_size
    sleep 10
    ./fdsconsole.py volume modify $vol --minimum 0 --maximum 0 --priority 1 
    popd
    sleep 10
    nbd_disk=`../../../cinder/nbdadm.py attach $node $vol`
}

function volume_detach {
    local vol=$1
    ../../../cinder/nbdadm.py detach $vol
}

function process_results {
    local f=$1
    local numjobs=$2
    local workload=$3
    local bs=$4
    local iodepth=$5
    local disksize=$6
    local nodes=$7

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
    echo nodes=$nodes >> .data
    ../common/push_to_influxdb.py fio_regr .data
}

#####################

outdir=$1
nodes=$2
node=luke
size="16m"
worker=8
workload="randread"
bs=4096
d=50

nbd_disk=""
volume_setup $bs $node volume_$bs
echo "nbd disk: $nbd_disk"

fio --name=write --rw=write --filename=$nbd_disk --bs=$bs --numjobs=1 --iodepth=32 --ioengine=libaio --direct=1 --size=$size

#sync
#echo 3 > /proc/sys/vm/drop_caches
echo fio --name=test --rw=$workload --filename=$nbd_disk --bs=$bs --numjobs=$worker --iodepth=$d --ioengine=libaio --direct=1 --size=$size --time_based --runtime=60
fio --name=test --rw=$workload --filename=$nbd_disk --bs=$bs --numjobs=$worker --iodepth=$d --ioengine=libaio --direct=1 --size=$size --time_based --runtime=60 | tee $outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size
process_results $outfile $worker $workload $bs $d $size $nodes
volume_detach volume_$bs

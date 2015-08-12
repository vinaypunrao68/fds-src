#!/bin/bash

#####################

function volume_setup {
    local max_obj_size=$1
    local node=$2
    local vol=$3
    pushd  ../../../cli
    let max_obj_size_kb=$max_obj_size/1024
    #./fds volume create -name $vol -type block -block_size $max_obj_size_kb -block_size_unit KB -media_policy HDD
    ./fds volume create -name $vol -type block -block_size 128 -block_size_unit KB -media_policy HDD
    sleep 10
    popd
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
    local start_time=$8
    local end_time=$9

    iops=`grep iops $f | sed -e 's/[ ,=:]/ /g' | awk '{e+=$7}END{print e}'`
    latency=`grep clat $f | grep avg| awk -F '[,=:()]' '{print ($2 == "msec") ? $9*1000 : $9}' | awk '{i+=1; e+=$1}END{print e/i/1000}'`

    echo file=$f > .data
    echo numjobs=$numjobs >> .data
    echo workload=$workload >> .data
    echo bs=$bs >> .data
    echo iodepth=$iodepth >> .data
    echo disksize=$disksize >> .data
    echo iops=$iops >> .data
    echo latency=$latency >> .data
    echo nodes=$nodes >> .data
    version=`dpkg -l|grep fds-platform | awk '{print $3}'` 
    echo version=$version >>.data
    echo start_time=$start_time >>.data
    echo end_time=$end_time >>.data
    ../common/push_to_influxdb.py fio_regr .data
    ../db/exp_db.py fio_regr_perf2 .data
}

#####################

outdir=$1
nodes=$2
node=$3
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
outfile=$outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size
start_time=`date +%s`
fio --name=test --rw=$workload --filename=$nbd_disk --bs=$bs --numjobs=$worker --iodepth=$d --ioengine=libaio --direct=1 --size=$size --time_based --runtime=60 | tee $outfile
end_time=`date +%s`
process_results $outfile $worker $workload $bs $d $size $nodes $start_time $end_time
volume_detach volume_$bs

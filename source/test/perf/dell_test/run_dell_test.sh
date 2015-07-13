#!/bin/bash

#########################

function volume_setup {
    local max_obj_size=$1
    local node=$2
    local vol=$3
    pushd ../../../cli
    let max_obj_size_kb=$max_obj_size/1024
    ./fds volume create -name $vol -type block -block_size $max_obj_size_kb -block_size_unit KB -media_policy HDD
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
    ../common/push_to_influxdb.py dell_test .data
}

#########################

outdir=$1
node=$2

size=8g

# Dell test specs:
# bsizes="512 4096 8192 65536 524288"
# iodepths="1 2 4 8 16 32 64 128 256"
# workers="4"
# workloads="randread read randwrite write"

#bsizes="4096"
bsizes="4096 8192 65536"
iodepths="32 64 128"
workers="4"
workloads="randread randwrite read write"


for bs in $bsizes ; do
    nbd_disk=""
    volume_setup $bs $node volume_$bs
    echo "nbd disk: $nbd_disk"
    if [ "$nbd_disk" = "" ]; then
        echo "Volume setup failed"
        exit 1;
    fi
    fio --name=write --rw=write --filename=$nbd_disk --bs=$bs --numjobs=4 --iodepth=64 --ioengine=libaio --direct=1 --size=$size
    for worker in $workers ; do
        for workload in $workloads ; do
                for d in $iodepths ; do
                #sync
                #echo 3 > /proc/sys/vm/drop_caches
                outfile=$outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size
                fio --name=test --rw=$workload --filename=$nbd_disk --bs=$bs --numjobs=$worker --iodepth=$d --ioengine=libaio --direct=1 --size=$size --time_based --runtime=60 | tee $outfile
                process_results $outfile $worker $workload $bs $d $size
                done
            done
        done
    volume_detach volume_$bs
done

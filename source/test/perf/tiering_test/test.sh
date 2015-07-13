#!/bin/bash
outdir=$1
tools=/fds/sbin
# policies="hdd ssd hybrid"
policies="HYBRID"

machines="luke han c3po chewie"

for p in $policies ; do
    pushd ../../../cli
    ./fds volume create -name volume_$p  -type block -block_size 4 -block_size_unit KB -media_policy $p

    if [ $? -ne 0 ]; then
        echo "fdscli has failed creating the volume"
        exit 1
    fi
    popd
    sleep 5
    disk=`$tools/nbdadm.py attach luke volume_$p`
    if [ $? -ne 0 ]; then
        echo "nbdadm failure"
        exit 1
    fi
    fio --name=write --rw=write --filename=$disk --bs=4096 --numjobs=1 --iodepth=32 --ioengine=libaio --direct=1 --size=8g
    if [ $? -ne 0 ]; then
        echo "FIO failure"
        exit 1
    fi
    for m in $machines ; do
        ssh $m 'sync && echo 3 > /proc/sys/vm/drop_caches'
    done
    fio --name=randread --rw=randread --filename=$disk --bs=4096 --numjobs=4 --iodepth=128 --ioengine=libaio --direct=1 --size=8g -time_based -runtime 120 | tee $outdir/out.$p
    if [ $? -ne 0 ]; then
        echo "FIO failure"
        exit 1
    fi
    sleep 5
done


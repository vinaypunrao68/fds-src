#!/bin/bash
outdir=$1
tools=/fds/sbin
policies="hdd ssd hybrid"
# policies="hybrid"

machines="luke han c3po chewie"

pushd $tools
./fdsconsole.py accesslevel admin
popd

for p in $policies ; do
    pushd $tools
    ./fdsconsole.py volume create volume_$p --vol-type block --blk-dev-size 10737418240 --media-policy $p --max-obj-size 4096
    popd
    sleep 5
    disk=`$tools/nbdadm.py attach luke volume_$p`
    fio --name=write --rw=write --filename=$disk --bs=4096 --numjobs=1 --iodepth=32 --ioengine=libaio --direct=1 --size=8g
    for m in $machines ; do
        ssh $m 'sync && echo 3 > /proc/sys/vm/drop_caches'
    done
    fio --name=randread --rw=randread --filename=$disk --bs=4096 --numjobs=4 --iodepth=128 --ioengine=libaio --direct=1 --size=8g -time_based -runtime 120 | tee $outdir/out.$p
    sleep 5
done

echo "Results"
for p in $policies ; do
    echo $p `grep iops results/out.$p | sed -e 's/[ ,=:]/ /g' | awk '{e+=$7}END{print e}'`
done

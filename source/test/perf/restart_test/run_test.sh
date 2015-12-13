#!/bin/bash

#########################

function volume_setup {
    local max_obj_size=$1
    local node=$2
    local vol=$3
    pushd /fds/sbin 
    # please use fdscli to create volumes
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
workspace=$2

size=$3
bs=4096

## # first start a system on both luke and han
## pushd $workspace/ansible
## ./scripts/deploy_fds.sh lukehan local
## popd

# Cleanup
# ../is_fds_up.py -K --fds-nodes han,luke,chewie,c3po
# rm -fr /fds/dev/*/*
# ssh han 'rm -fr /fds/dev/*/*'

$workspace/source/test/perf/is_fds_up.py -K --fds-nodes luke,chewie,han,c3po
for m in luke han c3po chewie ; do ssh $m rm -fr /fds/dev/*/*  ; done

cp $workspace/source/test/perf/restart_test/starwars $workspace/ansible/inventory/

# clean start system
pushd $workspace/ansible
./scripts/deploy_fds.sh starwars local
popd

sleep 10

# create volume
nbd_disk=""
volume_setup $bs luke volume_$bs
# load data
echo "nbd disk: $nbd_disk"
if [ "$nbd_disk" = "" ]; then
    echo "Volume setup failed"
    exit 1;
fi

fio --name=write --rw=write --filename=$nbd_disk --bs=$bs --numjobs=8 --iodepth=64 --ioengine=libaio --direct=1 --size=$size
sleep 1

# shutdown
pushd $workspace/source/cli
./fds local_domain shutdown -domain_name local
popd

sleep 600

#drop caches

sync
echo 3 > /proc/sys/vm/drop_caches

# restart
pushd $workspace/source/cli
./fds local_domain start -domain_name local
popd

sleep 60


date > $outdir/test.start

sleep 1

# launch monitoring script on the remote machines
for m in han chewie c3po ; do
    rem_mon_pids_$m=`ssh $m './monitor.sh /tmp/monitor'`
    echo rem_mon_pids_$m: "$rem_mon_pids_$m"
done

# launch monitoring script on the local machine
pushd $workspace/source/test/perf/migration_test
mon_pids=`./monitor.sh $outdir/monitor`
echo "mon_pids: $mon_pids"
popd

sleep 5

## counters ## pushd $workspace/source/test/perf/counters
## counters ## ./counters.py --agent-filter=sm &
## counters ## counters_pid=$!
## counters ## popd

# fio --name=randread --rw=randread --filename=$nbd_disk --bs=$bs --numjobs=4 --iodepth=128 --ioengine=libaio --direct=1 --size=$size --time_based --runtime=30 | tee $outdir/fio.out &
# fio_pid=$! 

resync=`grep 'Token resync on restart - done' /fds/var/logs/sm.log_1.log | wc -l`
 
while [ $resync -lt 1 ] ; do
    echo "not done yet"
    sleep 1
    resync=`grep 'Token resync on restart - done' /fds/var/logs/sm.log_1.log | wc -l`
done

# cntr=`grep -i migrat /fds/var/logs/sm.log_1.log |grep DLT |wc -l`
# while [ $cntr -lt 256 ] ; do
#     sleep 1
#     cntr=`grep -i migrat /fds/var/logs/sm.log_1.log |grep DLT |wc -l`
# done



date > $outdir/test.end

# wait $fio_pid

kill -9 $mon_pids
kill -9 $counters_pid
for m in han chewie c3po; do
    ssh $m kill -9 $"rem_mon_pids_$m"
    scp $m:/tmp/monitor.top $outdir/monitor.top.remote.$m
    scp $m:/tmp/monitor.iostat $outdir/monitor.iostat.remote.$m
    ssh $m 'rm -f /tmp/monitor.top'
    ssh $m 'rm -f /tmp/monitor.iostat'
done

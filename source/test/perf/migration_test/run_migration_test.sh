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

../is_fds_up.py -K --fds-nodes han,luke

rm -fr /fds/dev/*/*
ssh han 'rm -fr /fds/dev/*/*'

cp $workspace//source/test/perf/migration_test/luke $workspace/ansible/inventory/

# start system on luke
pushd $workspace/ansible
./scripts/deploy_fds.sh luke local
popd

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

# start pm on han
ssh han 'cd /fds/bin ; ulimit -n 400000 ; nohup ./platformd --fds-root=/fds --fds.pm.id=node1 --fds.pm.platform_port=7000 --fds.common.om_ip_list=10.1.10.222 > pm.out 2> pm.err < /dev/null &'
sleep 5
# activate sm
pushd /fds/sbin
pm_uuid=`./fdsconsole.py service list| grep PM |grep '10.1.10.139'| awk '{print \$1}'`
echo "pm uuid: $pm_uuid"
popd

sleep 5

# launch monitoring script on the local machine
pushd $workspace/source/test/perf/migration_test
mon_pids=`./monitor.sh $outdir/monitor`
echo "mon_pids: $mon_pids"
popd


sleep 5

#drop caches

sync
echo 3 > /proc/sys/vm/drop_caches

pushd /fds/sbin
# add node
./fdsconsole.py service addService $pm_uuid sm
popd

date > $outdir/migration.start

sleep 1

# launch monitoring script on the remote machine
rem_mon_pids=`ssh han './monitor.sh /tmp/monitor'`
echo "rem_mon_pids: $rem_mon_pids"

pushd $workspace/source/test/perf/counters
./counters.py --agent-filter=sm &
counters_pid=$!
popd

# fio --name=randread --rw=randread --filename=$nbd_disk --bs=$bs --numjobs=4 --iodepth=128 --ioengine=libaio --direct=1 --size=$size --time_based --runtime=30 | tee $outdir/fio.out &
# fio_pid=$! 

dlt_close=`grep 'Receiving DLT Close' /fds/var/logs/sm.log_0.log | wc -l`

while [ $dlt_close -lt 2 ] ; do
    sleep 1
    dlt_close=`grep 'Receiving DLT Close' /fds/var/logs/sm.log_0.log | wc -l`
done

date > $outdir/migration.end

# wait $fio_pid

kill -9 $mon_pids
kill -9 $counters_pid
ssh han "kill -9 $rem_mon_pids"
scp han:/tmp/monitor.top $outdir/monitor.top.remote
scp han:/tmp/monitor.iostat $outdir/monitor.iostat.remote
ssh han 'rm -f /tmp/monitor.top'
ssh han 'rm -f /tmp/monitor.iostat'

#!/bin/bash

#########################

SSH="sshpass -p passwd ssh -o StrictHostKeyChecking=no -l root"

function volume_setup {
    local vol=$1
    local policy=$2
    pushd ../../../cli
    echo "Creating: $vol"
    ./fds volume create -name $vol -type block -block_size 128 -block_size_unit KB -media_policy $policy
    popd
    sleep 10
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
    local machine=$7
    local vol=$8
    local start_time=$9
    local end_time=${10}
    local media_policy=${11}
    local exp_id=${12}

    version=`dpkg -l|grep fds-platform | awk '{print $3}'` 
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
    echo machine=$machine >> .data
    echo vol=$i >> .data
    echo version=$version >>.data
    echo start_time=$start_time >>.data
    echo end_time=$end_time >>.data
    echo media_policy=$media_policy >>.data
    echo exp_id=$exp_id >>.data
    ../common/push_to_influxdb.py dell_test .data --influxdb-db $database
    ../db/exp_db.py $database .data
}

#########################

outdir=$1
machines=$2
nvols=$3
am_machines=$4
size=$5
database=$6
media_policy=$7

echo "outdir: $outdir"
echo "machines: $machines"
echo "am_machines: $am_machines"
echo "size: $size"
echo "database: $database"
echo "media_policy: $media_policy"
# Dell test specs:
# bsizes="512 4096 8192 65536 524288"
# iodepths="1 2 4 8 16 32 64 128 256"
# workers="4"
# workloads="randread read randwrite write"

#bsizes="4096"
bsizes="4096 131072 524288"
iodepths="16 64"
workers="1 4"
workloads="randread randwrite read write"

echo "bsizes: $bsizes"
echo "iodepths: $iodepths"
echo "workers: $workers"
echo "workloads: $workloads"

declare -A disks

######### Remapping ##########
declare -A mremap
echo $machines
marray=($(echo ${machines}))
n_machines=${#marray[@]}
let i=$n_machines-1
for m in $machines ; do
    mremap[$m]=${marray[$i]}
    let i=$i-1
done

echo "machine - remap:"
for m in $machines; do echo "$m -> ${mremap[$m]}"; done
##############################
echo "Setting up volumes"
for m in $am_machines ; do
    for i in `seq $nvols` ; do
    	volume_setup volume_block_$m\_$i $media_policy

    	# disks[$m]=`../../../cinder/nbdadm.py attach $m  volume_block_$m`
	    echo "$m ->  ${mremap[$m]} $i"
    	disks[$m:$i]=`$SSH $m "cd /fds/sbin && ./nbdadm.py attach ${mremap[$m]}  volume_block_$m\_$i"`

    	echo "nbd disk for $m:$i connecting to  ${mremap[$m]}: ${disks[$m:$i]}"
    	if [ "${disks[$m:$i]}" = "" ]; then
    	    echo "Volume setup failed"
    	    exit 1;
    	fi
	    echo "writing from $m to ${disks[$m:$i]}"
    	$SSH $m "fio --name=write --rw=write --filename=${disks[$m:$i]} --bs=512k --numjobs=4 --iodepth=64 --ioengine=libaio --direct=1 --size=$size"
    done
done


declare -A pids
declare -A start_times
declare -A end_times
for bs in $bsizes ; do
    for worker in $workers ; do
        for workload in $workloads ; do
                for d in $iodepths ; do
                	#sync
                	#echo 3 > /proc/sys/vm/drop_caches
                    exp_id=`cat /regress/id`
                	for m in $am_machines ; do
    			        for i in `seq $nvols` ; do
                	    	outfile=$outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size.machine=$m.vol=$i
				            echo "reading from $m disk: ${disks[$m:$i]}"
                            start_times[$m:$i]=`date +%s%M`
                	    	$SSH $m "fio --name=test --rw=$workload --filename=${disks[$m:$i]} --bs=$bs --numjobs=$worker --iodepth=$d --ioengine=libaio --direct=1 --size=$size --time_based --runtime=60" | tee $outfile &
			    	        pids[$m:$i]=$!
                        done
			        done
                	for m in $am_machines ; do
    			        for i in `seq $nvols` ; do
			    	        echo "Waiting for $m ${pids[$m:$i]}"
			    	        wait ${pids[$m:$i]}
                            end_times[$m:$i]=`date +%s%M`
                	    	outfile=$outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size.machine=$m.vol=$i
			    	        echo "Processing results for $m ${pids[$m:$i]} $outfile"
                            echo "-> $worker $workload $bs $d $size $m $i ${start_times[$m:$i]} ${end_times[$m:$i]} $media_policy $exp_id"
                	        process_results $outfile $worker $workload $bs $d $size $m $i ${start_times[$m:$i]} ${end_times[$m:$i]} $media_policy $exp_id
                            start_times[$m:$i]=
                            end_times[$m:$i]=
			    	        pids[$m:$i]=""
                        done
			            sleep 10
                   done
                   let exp_id=$exp_id+1
                   echo $exp_id > /regress/id
                done
            done
        done
done
for m in $am_machines ; do
    for i in `seq $nvols` ; do
        volume_detach volume_block_$m\_$i
    done
done

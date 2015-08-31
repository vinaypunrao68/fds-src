#!/bin/bash

#########################
# Functions and misc
#########################

SSH="sshpass -p passwd ssh -o StrictHostKeyChecking=no -l root"

# set up a volume given its name
function volume_setup {
    local vol=$1
    local policy=$2
    pushd ../../../cli
    echo "Creating: $vol"
    ./fds volume create -name $vol -type block -block_size 128 -block_size_unit KB -tiering_policy $policy
    popd
    sleep 10
}

# detach a volume given its name
function volume_detach {
    local vol=$1
    ../../../cinder/nbdadm.py detach $vol
}

# process FIO results and push to influxdb and mysql
# add here your result resporting backend (see 
# end of the function)
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
    local tag=${13}

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
    echo tag=$tag >>.data
    # .data has the results - Add here your result reporting backend
    echo "Results:"
    cat .data
    # Uncomment this if you don't want to push to influxdb
    ../common/push_to_influxdb.py dell_test .data --influxdb-db $database
    # Uncomment this if you don't want to push to mysql
    ../db/exp_db.py $database .data
}

#########################
# Main script
#########################

# FDS cluster
machines=$1     # Machine where the test will run                Ex.: "perf2-node1 perf2-node2 perf2-node3 perf2-node4"
am_machines=$2  # Machines targeted by FIO (number of AMs)       Ex.: "perf2-node1 perf2-node2 perf2-node3 perf2-node4"
nvols=$3        # Number of volumes per AM                       Ex.: 2
media_policy=$4 # Media polcy                                   Ex.: HDD
# FIO parameters
outdir=$5       # Output directory for FIO (must exist)          Ex.: "/regress/test"
size=$6         # Size of each volume                            Ex.: 50g
# Parameter spaces - each value is an array and will test all combinations
bsizes=$7       # block sizes                                    Ex.: "4096 131072 524288"
iodepths=$8     # iodepth                                        Ex.: "16 64"
workers=$9      # Numer of jobs                                  Ex.: "1 4"
workloads=${10}    # Workloads                                     Ex.: "randread randwrite read write"
# Results reporting (Matteo's perf reporting)
database=${11}    # Name of the database for the results (influx and mysql) Ex.: perf
tag=${12}       # Optional: some custom tag: ie: my_custom_test

if [[ ( -z "$1" ) || ( -z "$2" ) || ( -z "$3" )  || ( -z "$4" )   || ( -z "$5" ) || ( -z "$6" )   || ( -z "$7" )  || ( -z "$8" )  || ( -z "$9" ) || ( -z "$10" )  || ( -z "$11" )]]
  then
    echo "Usage ./program list of parameters. Look at the comments in the code for detailed usage"
    exit 1
fi

echo "outdir: $outdir"
echo "machines: $machines"
echo "am_machines: $am_machines"
echo "size: $size"
echo "media_policy: $media_policy"
echo "bsizes: $bsizes"
echo "iodepths: $iodepths"
echo "workers: $workers"
echo "workloads: $workloads"
echo "database: $database"
echo "tag: $tag"

# Dell test specs:
# bsizes="512 4096 8192 65536 524288"
# iodepths="1 2 4 8 16 32 64 128 256"
# workers="4"
# workloads="randread read randwrite write"

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

######### Setting volumes up  ##########

echo "Setting up volumes"
for m in $am_machines ; do
    for i in `seq $nvols` ; do
    	volume_setup volume_block_$m\_$i $media_policy

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

######### Testing  ##########

declare -A pids
declare -A start_times
declare -A end_times
for bs in $bsizes ; do
    for worker in $workers ; do
        for workload in $workloads ; do
                for d in $iodepths ; do
                    # If you want to drop the FS cache uncomment
                	#sync
                	#echo 3 > /proc/sys/vm/drop_caches
                    exp_id=`cat /regress/id`
                	for m in $am_machines ; do
    			        for i in `seq $nvols` ; do
                	    	outfile=$outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size.machine=$m.vol=$i
				            echo "reading from $m disk: ${disks[$m:$i]}"
                            start_times[$m:$i]=`date +%s`
                	    	$SSH $m "fio --name=test --rw=$workload --filename=${disks[$m:$i]} --bs=$bs --numjobs=$worker --iodepth=$d --ioengine=libaio --direct=1 --size=$size --time_based --runtime=60" | tee $outfile &
			    	        pids[$m:$i]=$!
                        done
			        done
                	for m in $am_machines ; do
    			        for i in `seq $nvols` ; do
			    	        echo "Waiting for $m ${pids[$m:$i]}"
			    	        wait ${pids[$m:$i]}
                            end_times[$m:$i]=`date +%s`
                	    	outfile=$outdir/out.numjobs=$worker.workload=$workload.bs=$bs.iodepth=$d.disksize=$size.machine=$m.vol=$i
			    	        echo "Processing results for $m ${pids[$m:$i]} $outfile"
                            echo "-> $worker $workload $bs $d $size $m $i ${start_times[$m:$i]} ${end_times[$m:$i]} $media_policy $exp_id"
                	        process_results $outfile $worker $workload $bs $d $size $m $i ${start_times[$m:$i]} ${end_times[$m:$i]} $media_policy $exp_id $tag
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

######### Detaching volumes  ##########

for m in $am_machines ; do
    for i in `seq $nvols` ; do
        volume_detach volume_block_$m\_$i
    done
done

#!/bin/bash

function s3_setup {
    local node=$1
    local policy=$2
    echo "Setting up s3 on $node"
    pushd ../../../cli
    ./fds volume create -name volume0 -type object -max_object_size 128 -max_object_size_unit KB -tiering_policy $policy
    sleep 10
    popd
}

function process_results {
    local files=$1
    local n_reqs=$2
    local n_files=$3
    local outs=$4
    local test_type=$5
    local object_size=$6
    local hostname=$7
    local n_conns=$8
    local n_jobs=$9
    local start_time=${10}
    local end_time=${11}
    local media_policy=${12}

    iops=`echo $files | xargs grep IOPs |awk '{e+=$2}END{print e}'`
    latency=`echo $files | xargs grep latency | awk '{print $4*1e-6}'|awk '{i+=1; e+=$1}END{print e/i}'`

    echo file=$f > .data
    echo n_reqs=$n_reqs >> .data
    echo n_files=$n_files >> .data
    echo outs=$outs >> .data
    echo test_type=$test_type >> .data
    echo object_size=$object_size >> .data
    echo hostname=$hostname >> .data
    echo n_conns=$n_conns >> .data
    echo n_jobs=$n_jobs >> .data
    echo iops=$iops >>.data
    echo latency=$latency >>.data
    version=`dpkg -l|grep fds-platform | awk '{print $3}'` 
    echo version=$version >>.data
    echo start_time=$start_time >>.data
    echo end_time=$end_time >>.data
    ../common/push_to_influxdb.py s3_test .data
    ../db/exp_db.py s3_test .data
}

##################################

outdir=$1
workspace=$2
media_policy=$3

client=perf2-node4
njobs=4

n_reqs=100000 
n_files=1000 
outs=100 
test_type="GET"
object_size=4096 
hostname="perf2-node1" 
n_conns=100 
n_jobs=4

test_types="PUT"
object_sizes="4096 65536 1048576"
concurrencies="100"

s3_setup perf2-node1 $media_policy

#FIXME: assuming trafficgen is installed on the client
#pushd $workspace/source/Build/linux-x86_64.release
#tar czvf java_tools.tgz tools lib/java
#popd
#scp $workspace/source/Build/linux-x86_64.release/java_tools.tgz $client:/root/java_tools.tgz
#ssh $client 'tar xzvf java_tools.tgz'
name=`whoami`
echo whoami

for t in $test_types ; do
    for o in $object_sizes ; do
        for c in $concurrencies ; do
            test_type=$t
            object_size=$o
            n_conns=$c
            outs=$c

            cmd="cd /root/tools; ./trafficgen.py --num-requests $n_reqs --num-files $n_files --threads $outs --type $test_type --file-size $object_size --target-node $hostname"

            pids=""
            outfiles=""
            start_time=`date +%s`
            for j in `seq $n_jobs` ; do
                f=$outdir/out.n_reqs=$n_reqs.n_files=$n_files.outstanding_reqs=$outs.test_type=$test_type.object_size=$object_size.hostname=$hostname.n_conns=$n_conns.job=$j
                outfiles="$outfiles $f"
                ssh $client "$cmd"  | tee $f &
                pid="$pid $!"
                pids="$pids $!"
            done
            wait $pids
            end_time=`date +%s`
            process_results "$outfiles" $n_reqs $n_files $outs $test_type $object_size $hostname $n_conns $n_jobs $start_time $end_time
        done
    done
done

test_types="GET"
for t in $test_types ; do
    for o in $object_sizes ; do
        for c in $concurrencies ; do
            test_type=$t
            object_size=$o
            n_conns=$c
            outs=$c

            cmd="cd /root/tools; ./trafficgen.py --num-requests $n_reqs --num-files $n_files --threads $outs --type $test_type --file-size $object_size --target-node $hostname"

            pids=""
            outfiles=""
            for j in `seq $n_jobs` ; do
                f=$outdir/out.n_reqs=$n_reqs.n_files=$n_files.outstanding_reqs=$outs.test_type=$test_type.object_size=$object_size.hostname=$hostname.n_conns=$n_conns.job=$j
                outfiles="$outfiles $f"
                ssh $client "$cmd"  | tee $f &
                pid="$pid $!"
                pids="$pids $!"
            done
            wait $pids
            end_time=`date +%s`
            process_results "$outfiles" $n_reqs $n_files $outs $test_type $object_size $hostname $n_conns $n_jobs $start_time $end_time
        done
    done
done

test_types="MULTIPART"
echo $test_types
#Set object size to larger amount for multipart uploads
object_sizes="33554432"
for t in $test_types ; do
    for o in $object_sizes ; do
        for c in $concurrencies ; do
            test_type=$t
            object_size=$o
            n_conns=$c
            outs=$t


            cmd="cd /root/tools; ./trafficgen.py --num-requests $n_reqs --num-files $n_files --threads $outs --type $test_type --file-size $object_size --target-node $hostname"

            pids=""
            outfiles=""
            for j in `seq $n_jobs` ; do
                f=$outdir/out.n_reqs=$n_reqs.n_files=$n_files.outstanding_reqs=$outs.test_type=$test_type.object_size=$object_size.hostname=$hostname.n_conns=$n_conns.job=$j
                outfiles="$outfiles $f"
                ssh $client "$cmd"  | tee $f &
                pid="$pid $!"
                pids="$pids $!"
            done
            wait $pids
            end_time=`date +%s`
            process_results "$outfiles" $n_reqs $n_files $outs $test_type $object_size $hostname $n_conns $n_jobs $start_time $end_time
        done
    done
done


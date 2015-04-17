#!/bin/bash

##################################
function s3_setup {
    local node=$1
    echo "Setting up s3 on $node"
    pushd /fds/sbin && ./fdsconsole.py accesslevel debug && ./fdsconsole.py volume create volume0 --vol-type object && popd
    sleep 10
}
# process_results $outfile $n_reqs $n_files $outs $test_type $object_size $hostname $n_conns $n_jobs
function process_results {
    local f=$1
    local n_reqs=$2
    local n_files=$3
    local outs=$4
    local test_type=$5
    local object_size=$6
    local hostname=$7
    local n_conns=$8
    local n_jobs=$9

    iops=`grep iops $f | sed -e 's/[ ,=:]/ /g' | awk '{e+=$7}END{print e}'`
    latency=`grep clat $f | grep avg| awk -F '[,=:()]' '{print $9}' | awk '{i+=1; e+=$1}END{print e/i}'`

    echo file=$f > .data
    echo n_reqs=$n_reqs >> .data
    echo n_files=$n_files >> .data
    echo outs=$outs >> .data
    echo test_type=$test_type >> .data
    echo object_size=$object_size >> .data
    echo hostname=$hostname >> .data
    echo n_conns=$n_conns >> .data
    echo n_jobs=$n_jobs >> .data
    ../common/push_to_influxdb.py s3_test .data
}

##################################

outdir=$1
workspace=$2

client=han
njobs=4

n_reqs=1000000 
n_files=1000 
outs=100 
test_type="GET"
object_size=4096 
hostname="luke" 
n_conns=100 
n_jobs=4

test_types="GET"
object_sizes="4096"
concurrencies="100"

s3_setup luke

cd $workspace/source/Build/linux-x86_64.release
tar czvf java_tools.tgz tools lib/java

scp $workspace/source/Build/linux-x86_64.release/java_tools.tgz $client:/root/java_tools.tgz
ssh $client 'tar xzvf java_tools.tgz'

echo "loading dataset"
cmd="cd /root/tools; ./trafficgen --n_reqs 20000 --n_files 1000 --outstanding_reqs 50 --test_type PUT --object_size 4096 --hostname luke --n_conns 50"
ssh $client "$cmd"

cmd="cd /root/tools; ./trafficgen --n_reqs 1000000 --n_files 1000 --outstanding_reqs 100 --test_type GET --object_size 4096 --hostname luke --n_conns 100"

for t in $test_types ; do
    for o in $object_sizes ; do
        for c in $concurrencies ; do
            test_type=$t
            object_size=$o
            n_conns=$c
            outs=$c

            cmd="cd /root/tools; ./trafficgen --n_reqs $n_reqs --n_files $n_files --outstanding_reqs $outs --test_type $test_type --object_size $object_size --hostname $hostname --n_conns $n_conns"

            pids=""
            for j in `seq $njobs` ; do
                ssh $client "$cmd"  | tee $outdir/out.n_reqs=$n_reqs.n_files=$n_files.outstanding_reqs=$outs.test_type=$test_type.object_size=$object_size.hostname=$hostname.n_conns=$nconns.job=$j &
                pid="$pid $!"
                pids="$pids $!"
            done
            wait $pids
            process_results $outfile $n_reqs $n_files $outs $test_type $object_size $hostname $n_conns $n_jobs
        done
    done
done




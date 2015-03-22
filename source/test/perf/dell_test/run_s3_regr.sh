#!/bin/bash

outdir=$1
workspace=$2

cd $workspace/source/Build/linux-x86_64.release
tar czvf java_tools.tgz tools lib/java

scp $workspace/source/Build/linux-x86_64.release/java_tools.tgz han:/root/java_tools.tgz
ssh han 'tar xzvf java_tools.tgz'

echo "loading dataset"
cmd="cd /root/tools; ./trafficgen --n_reqs 20000 --n_files 1000 --outstanding_reqs 50 --test_type PUT --object_size 4096 --hostname luke --n_conns 50"
ssh han "$cmd"

cmd="cd /root/tools; ./trafficgen --n_reqs 1000000 --n_files 1000 --outstanding_reqs 100 --test_type GET --object_size 4096 --hostname luke --n_conns 100"

pids=""
for j in 1 2 3 4 ; do
    ssh han "$cmd"  | tee $outdir/out.n_reqs=1000000.n_file=1000.outstanding_reqs=100.test_type=GET.object_size=4096.hostname=luke.n_conns=100.job=$j &
    pid="$pid $!"
    pids="$pids $!"
done
wait $pids



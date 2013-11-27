#!/bin/bash
#

pushd .
cd ../test
python bring_up.py --up --file=tier_sprint_demo_hybrid.cfg
popd
sleep 5

#
# first, populte volumes so we can read objs
# 
./run-delay-workloads.sh -p "tier-demo-initial-write" -w "sample_workloads/seq_write.sh:/dev/fbd0" -w "sample_workloads/seq_write.sh:/dev/fbd1" -w "sample_workloads/seq_write.sh:/dev/fbd2" -w "sample_workloads/seq_write.sh:/dev/fbd3"

sleep 5

#
# now run 3 sequential over large region and 1 sequential that loops over first 2 MB of the device
# We should see that vol5 will start getting its reads from flash after some time
# 
./run-delay-workloads.sh -p "tier-demo" -w "sample_workloads/seq_read_loop.sh:/dev/fbd3" -w "sample_workloads/seq_read.sh:/dev/fbd1" -w "sample_workloads/seq_read.sh:/dev/fbd2" -w "sample_workloads/seq_read.sh:/dev/fbd0"


mv results results-tier-demo


pushd .
cd ../test
python bring_up.py --down --file=tier_sprint_demo_hybrid.cfg
popd





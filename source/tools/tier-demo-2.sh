#!/bin/bash
#
# Not shown during tier sprint demo.
# Another demo for hybrid_prefcap policy -- this policy 
# prefers the capacity tier (disk), so the first put always 
# goes to disk. However, hot objects get promoted. To see the effect
# of this policy -- make sure to compile stor mgr with large rank table,
# at least 100000. 
#


# We should cleanup for this experiment, because otput we assuming 
#
# this policy has volumes #2,3,4 that are hybrid and volumes #5 is hybrid_prefcap
# 
pushd .
cd ../test
python bring_up.py --up --file=tier_test_hybrid_prefcap.cfg
popd
sleep 5

#
# first, populte volumes so we can read objs
# Assuming our stor mgr is compiled with large rank table size, we
# should see that all writes to volumes #2,3,4 (on iops graph, that would be numbers 2,3,4, as well)
# go to flash. All writes to volume #5 (on iops graph its label is 1) should go to disk.
./run-delay-workloads.sh -p "tier-demo-initial-write" -w "sample_workloads/seq_write.sh:/dev/fbd1" -w "sample_workloads/seq_write.sh:/dev/fbd2" -w "sample_workloads/seq_write.sh:/dev/fbd3" -w "sample_workloads/seq_write.sh:/dev/fbd4"

sleep 5

#
# now run 3 sequential over large region and 1 sequential (volume id 5) that loops over first 2 MB of the device
# we should see that vols with ids 2,3,4 get all reads from flash (since rank table size is large, it should not be
# filled yet. We should see that vol5 will start getting its reads from flash after some time
# 
./run-delay-workloads.sh -p "tier-demo" -w "sample_workloads/seq_read_loop.sh:/dev/fbd4" -w "sample_workloads/seq_read.sh:/dev/fbd2" -w "sample_workloads/seq_read.sh:/dev/fbd3" -w "sample_workloads/seq_read.sh:/dev/fbd1"


mv results results-tier-demo


pushd .
cd ../test
python bring_up.py --down --file=tier_test_hybrid_prefcap.cfg
popd





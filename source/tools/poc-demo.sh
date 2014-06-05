#!/bin/bash
#

# NOTE that volume6 is s3 volume that we had to
# populate and run separately.

#pushd .
#cd ../test
#python bring_up.py --up --file=poc_demo.cfg -v
#popd
#sleep 15

# initial write (short)
./run-delay-workloads.sh -p "poc-demo-write" -w "sample_workloads/seq_write.sh:/dev/nbd0"

#sleep 15

# read sequentially data we wrote
#./run-delay-workloads.sh -p "poc-demo-read" -w "sample_workloads/seq_read.sh:/dev/fbd0" -w "sample_workloads/seq_read.sh:/dev/fbd2" -w "sample_workloads/seq_read.sh:/dev/fbd2" -w "sample_workloads/seq_read.sh:/dev/fbd3"

#mv results results-poc-demo

# during POC demo, we commented out the bellow call to bring everything down
# and had another script that just did read (and ./run-delay-workloads.sh above)
# so that we didn't have to re-populate volume6 again

#pushd .
#cd ../test
#python bring_up.py --down --file=poc_demo.cfg -v
#popd

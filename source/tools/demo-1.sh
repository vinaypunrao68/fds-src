#!/bin/bash
#

pushd .
cd ../test
python bring_up.py --up --file=qos_demo_ssd.cfg -v
popd
sleep 10

./run-delay-workloads.sh -p "qos-write" -w "sample_workloads/seq_write.sh:/dev/fbd0" -w "sample_workloads/seq_write.sh:/dev/fbd1" -w "sample_workloads/seq_write.sh:/dev/fbd2" -w "sample_workloads/seq_write.sh:/dev/fbd3"

sleep 15

./run-delay-workloads.sh -p "qos-read" -w "sample_workloads/seq_read.sh:/dev/fbd0" -w "sample_workloads/seq_read.sh:/dev/fbd1" -w "sample_workloads/seq_read.sh:/dev/fbd2" -w "sample_workloads/seq_read.sh:/dev/fbd3"

mv results results-demo

pushd .
cd ../test
python bring_up.py --down --file=qos_demo_ssd.cfg
popd



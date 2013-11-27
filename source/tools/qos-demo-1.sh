#!/bin/bash
#

./run-delay-workloads.sh -p "qos-demo-1" -w "sample_workloads/seq_write.sh:/dev/fbd0" -w "sample_workloads/seq_write.sh:/dev/fbd1" -w "sample_workloads/seq_write.sh:/dev/fbd2" -w "sample_workloads/seq_write.sh:/dev/fbd3"

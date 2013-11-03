#!/bin/bash
#

./run-delay-workloads.sh -p "qos-demo-1" -w "sample_workloads/seq_write.sh:/dev/fbd1" -w "sample_workloads/seq_write.sh:/dev/fbd2" -w "sample_workloads/seq_write.sh:/dev/fbd3" -w "sample_workloads/seq_write.sh:/dev/fbd4"

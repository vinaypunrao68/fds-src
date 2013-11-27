#!/bin/bash
#

./run-delay-workloads.sh -p "max60-min100-write" -w "sample_workloads/seq_write.sh:/dev/fbd0" -w "sample_workloads/seq_write.sh:/dev/fbd1" -w "sample_workloads/seq_write.sh:/dev/fbd4" -w "sample_workloads/seq_write.sh:/dev/fbd3"

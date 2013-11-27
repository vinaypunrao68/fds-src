#!/bin/bash
#

./run-delay-workloads.sh -p "p1max60-read" -w "sample_workloads/seq_read.sh:/dev/fbd4" -w "sample_workloads/seq_read.sh:/dev/fbd0" -w "sample_workloads/seq_read.sh:/dev/fbd1" -w "sample_workloads/seq_read.sh:/dev/fbd2"

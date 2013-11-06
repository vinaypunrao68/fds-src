#!/bin/bash
#

./run-delay-workloads.sh -p "p1max60-read" -w "sample_workloads/seq_read.sh:/dev/fbd5" -w "sample_workloads/seq_read.sh:/dev/fbd1" -w "sample_workloads/seq_read.sh:/dev/fbd2" -w "sample_workloads/seq_read.sh:/dev/fbd3"

#!/bin/bash
#

#
# Proposal for a tier workloads
#

# assumes we already ran qos-demo-1.sh and sequentially write to ssds
# then we wait for the migrator to run and demote some objects 
# 

#
# now run 3 sequential over large region and 1 sequential that loops over first 2 MB of the device
#
./run-delay-workloads.sh -p "tier-demo-1" -w "sample_workloads/seq_read.sh:/dev/fbd1" -w "sample_workloads/seq_read.sh:/dev/fbd2" -w "sample_workloads/seq_read.sh:/dev/fbd3" -w "sample_workloads/seq_read_loop.sh:/dev/fbd4"

#!/bin/bash
#change to ip of the node to upload
ip=127.0.0.1

#Change to bucket name
bucket=volume

cnt=1
# Create buckets
for f in `seq 1 $cnt`; do curl -v -X POST http://$ip:8000/${bucket}_${f}; done

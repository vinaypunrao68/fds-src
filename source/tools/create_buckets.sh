#!/bin/bash
#change to ip of the node to upload
ip=127.0.0.1
port=8000

#Change to bucket name
bucket=smoke_volume

cnt=6
# Create buckets
for f in `seq 0 $cnt`; do curl -v -X PUT http://$ip:8000/${bucket}${f}; done
#curl -v -X PUT http://${ip}:${port}/${bucket}

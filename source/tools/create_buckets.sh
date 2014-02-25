#!/bin/bash
#change to ip of the node to upload
ip=127.0.0.1
port=8000

#Change to bucket name
bucket=volume
bucket=atmos

cnt=1
# Create buckets
#for f in `seq 1 $cnt`; do curl -v -X POST http://$ip:8000/${bucket}_${f}; done
curl -v -X POST http://${ip}:${port}/${bucket}

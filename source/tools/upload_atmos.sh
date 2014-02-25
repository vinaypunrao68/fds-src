#!/bin/bash

# change this to the directory containing images
files_dir=~/temp/skinet

#change to ip of the node to upload
ip=127.0.0.1
port=8001
port_s3=8000

#Change to bucket name
bucket=rest/objects
atmos_bucket=atmos

# Get the first 80 files
cd $files_dir
flist=`ls | head -1`
#flist=`ls`

# Create bucket, hack to create the initial bucket
curl -v -X POST http://$ip:${port_s3}/$atmos_bucket

# Post images
for f in $flist; do
    echo curl -v -X POST --data-binary @$f http://$ip:${port}/$bucket/$f;
    curl -v -X POST --data-binary @$f http://$ip:${port}/$bucket/$f;
done

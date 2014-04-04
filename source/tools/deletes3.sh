#!/bin/bash

# change this to the directory containing images
files_dir=~/temp/skinet2
files_dir=/smoke_test_smallfiles
files_dir=/dataset/rand_100MB_files

#change to ip of the node to upload
#ip=localhost
ip=10.1.10.115

#Change to bucket name
bucket=smoke_volume0

# Get the first 80 files
cd $files_dir
flist=`ls | head -80`
#flist=`ls`
#flist=' find . -type f  -print'

# Create bucket
#curl -v -X POST http://$ip:8000/$bucket

# Post images
for f in $flist; do
    curl -v -X DELETE http://$ip:8000/$bucket/$f;
done

#!/bin/bash

# change this to the directory containing images
files_dir=~/temp/skinet

#change to ip of the node to upload
ip=127.0.0.1

#Change to bucket name
bucket=volume2

# Get the first 80 files
cd $files_dir
flist=`ls | head -80`

# Create bucket
curl -v -X POST http://$ip:8000/$bucket

# Post images
for f in $flist; do curl -v -X POST --data-binary @$f http://$ip:8000/$bucket/$f; done


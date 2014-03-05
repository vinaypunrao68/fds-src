#!/bin/bash

# change this to the directory containing images
files_dir=~/temp/skinet
res_dir=~/temp/res
rm -rf $res_dir
mkdir -p $res_dir

#change to ip of the node to upload
ip=127.0.0.1

#Change to bucket name
bucket=volume_smoke1

# Get the first 80 files
cd $files_dir
flist=`ls`

# Create bucket
#curl -v -X POST http://$ip:8000/$bucket

# Post images
for f in $flist; do
    checksum=`md5sum $f | awk '{print $1}'`
    #curl -v -X GET http://$ip:8000/$bucket/$f > $res_dir/$f
    echo "curl -X GET http://$ip:8000/$bucket/$f > $res_dir/$f"
    curl -X GET http://$ip:8000/$bucket/$f > $res_dir/$f
    res_sum=`md5sum $res_dir/$f | awk '{print $1}'`
    echo $checksum
    echo $res_sum
    if [ $checksum != $res_sum ]; then
        echo "Checksum mismatched: file $f - $checksum != $res_sum"
        exit 1
    fi
done

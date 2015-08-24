#!/bin/bash

fsize=$1 # MB
basedir=$2

if [[ ( -z "$1" ) || ( -z "$2"  ) ]]
  then
    echo "Usage: ./program <filesize in MB> <basedir>"
    exit 1
fi

echo "creating file..."
dd if=/dev/urandom of=$basedir/file_$fsize bs=1048576 count=$fsize
chmod 755 $basedir/*

#!/bin/bash

fsize=$1 # MB
basedir=$2
echo "creating files..."
dd if=/dev/urandom of=$basedir/file_$fsize bs=1048576 count=$fsize
chmod 755 $basedir/*

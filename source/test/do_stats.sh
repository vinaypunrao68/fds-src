#!/bin/bash
directory=$1
flags=$2
for name in `ls $directory/out.* | sed 's/.*out\.*//'` 
do
    ./get_stats.py -d $directory -n $name $flags
done

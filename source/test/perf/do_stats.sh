#!/bin/bash
base=$1
node=$2
for dir in `ls $base` 
do
    #name=`echo $dir| awk -F "[.:]" '{print "type, ",$6, ", nvols,",$2,", threads,",$4," , fsize,",$12}'`
    name=$dir
    ./get_stats.py -d $base/$dir -n "$name" --fds-nodes $node
done

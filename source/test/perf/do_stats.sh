#!/bin/bash
base=$1
node=$2
db=$3
node=$4
config=$5
mode=$6
if [ "$mode" = "fio" ]
then
    options="-F"
elif [ "$mode" = "s3_java" ] 
then
    options="-j"
else
    options=""
fi
for dir in `ls $base|grep results_` 
do
    #name=`echo $dir| awk -F "[.:]" '{print "type, ",$6, ", nvols,",$2,", threads,",$4," , fsize,",$12}'`
    name=$dir
    ./get_stats.py -d $base/$dir -n "$name" --fds-nodes $node -b $db -N $node -c $config $options
done

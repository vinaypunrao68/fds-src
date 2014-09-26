#!/bin/bash

results=$1

#disks=`ls /dev/sd?` 
disks="/dev/nbd15"
bsizes="4k"
echo $disks
options="--runtime=30 --ioengine=libaio --iodepth=16 --direct=1 --size=10g --numjobs=1 "
writes="--name=writes $options --rw=randwrite"
reads="--name=reads $options --rw=randread"

if [ ! -d $results ]
then
    mkdir $results
fi

for bs in $bsizes
do    
    for d in $disks
    do
      target=`echo $d | awk -F "/" '{print $3}'`
      echo fio $writes --filename=$d --bs=$bs --output $results/writes.$target.fio
      fio $writes --filename=$d --bs=$bs --output $results/writes.$target.$bs.fio
      echo fio $reads --filename=$d --bs=$bs --output $results/reads.$target.$bs.fio
      fio $reads --filename=$d --bs=$bs --output $results/reads.$target.fio
    done
done

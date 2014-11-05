#!/bin/bash

results=$1
disk_target=$2
iodirect=$3


#disks=`ls /dev/sd?` 
disks="$disk_target"
bsizes="4k"
ags="8 16 32 64 128 256"
njobs="4 8 16 32 64 128 256"

echo $disks
options="--runtime=30 --ioengine=libaio --iodepth=16 --direct=$iodirect --size=10g  "
seq_writes="--name=seq_writes $options --rw=write"
seq_reads="--name=seq_reads $options --rw=read"
seq_readwrites="--name=seq_readwrites $options --rw=readwrite"
rand_writes="--name=rand_writes $options --rw=randwrite"
rand_reads="--name=rand_reads $options --rw=randread"
rand_readwrites="--name=rand_readwrites $options --rw=randrw"
output_format="--minimal"

if [ ! -d $results ]
then
    mkdir $results
fi

for bs in $bsizes
do
    for d in $disks
    do
        for ag in $ags
        do
            for job in $njobs  
            do    
                mkfs.xfs -f $d -b size=4096 -d agcount=$ag
                #mkfs.xfs -f $d 
                mount $d /home/monchier/tmp
                xfs_info $d
                target=`echo $d | awk -F "/" '{print $3}'`
                fio $seq_writes --filename=$d --bs=$bs --numjobs=$job $output_format  --output $results/seq_writes.$target.$bs.$ag.$job.fio
                fio $seq_reads --filename=$d --bs=$bs --numjobs=$job $output_format  --output $results/seq_reads.$target.$bs.$ag.$job.fio
                fio $seq_readwrites --filename=$d --bs=$bs --numjobs=$job $output_format  --output $results/seq_readwrites.$target.$bs.$ag.$job.fio
                # 
                fio $rand_writes --filename=$d --bs=$bs --numjobs=$job $output_format  --output $results/rand_writes.$target.$bs.$ag.$job.fio
                fio $rand_reads --filename=$d --bs=$bs --numjobs=$job $output_format  --output $results/rand_reads.$target.$bs.$ag.$job.fio
                fio $rand_readwrites --filename=$d --bs=$bs --numjobs=$job $output_format  --output $results/rand_readwrites.$target.$bs.$ag.$job.fio
                umount $d
            done
        done
    done
done

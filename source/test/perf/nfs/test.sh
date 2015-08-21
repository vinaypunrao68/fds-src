#!/bin/bash

fsize=$1 # size in kB or "list" 
fnumber=$2 # number of files to be generated or path to "list"
concurrency=$3
host=$4
echo "fsize: $fsize"
echo "fnumber: $fnumber"
echo "concurrency: $concurrency"
echo "Host: $host"
nfs_bsize=1048576
basedir=/root
target=/mount/tmp

#rsync="rsync -av --progress"
#rsync="rsync -a"
rsync=cp

timestamp() {
      date +"%s%3N"
}

rm -f .files
rm -f .cmds

if [ "$fsize" == "list" ]; then
    path=$fnumber
    cp $path .files
else
    # generate many small files
    
    if [ ! -d $basedir/small_files ] ; then
        mkdir -p $basedir/small_files
    else
        echo "cleaning up"
        rm -f  $basedir/small_files/*
    fi
    
    #declare -a checksum
    > .files
    echo "creating files..."
    for i in `seq 0 $(($fnumber-1))` ; do
        dd if=/dev/urandom of=$basedir/small_files/file$i bs=1024 count=$fsize
        #checksum[$i]=`md5sum $basedir/small_files/file$i`
        chmod 755 $basedir/small_files/file$i
        echo $basedir/small_files/file$i >> .files
    done
fi
bytes=0
for f in `cat .files` ; do
    size=`ls -l $f |grep -v total | awk '{print $5}'`
    bytes=$(($bytes+$size))
done
echo "Total size [byte]: $bytes"

# create fds volume
volume="test"
fds volume create -name $volume -type block -block_size 1024 -block_size_unit KB -media_policy HDD
sleep 10
cmd="mount -t nfs -onfsvers=3,wsize=$nfs_bsize,rsize=$nfs_bsize $host:/$volume $target"
echo "Mounting: $cmd"
$cmd
if [ $? -ne 0 ]; then
    echo "mount error"
    exit 1
fi
# do the write test

> .cmds 
for f in `cat .files` ; do
    fname=`basename $f`
    cmd="$rsync $f $target/$fname"
    echo $cmd >> .cmds
done

echo "Write Test"

start_time=`timestamp`
./concurrent_test.py .cmds $concurrency
end_time=`timestamp`

echo "Write Test Done"

elapsed=$(($end_time-$start_time))
echo "Elapsed[ms]=$elapsed"
echo "Bytes=$bytes"
bw=` bc <<< "scale=2; $bytes/$elapsed*1000/1024/1024"`
echo "BW[MB/s]=$bw"

# do the read test

if [ ! -d $basedir/files_read ] ; then
    mkdir -p $basedir/files_read
else
    echo "cleaning up"
    rm -f  $basedir/files_read/*
fi

> .cmds 
for f in `cat .files` ; do
    fname=`basename $f`
    cmd="$rsync $target/$fname $basedir/files_read/$fname"
    echo $cmd >> .cmds
done

echo "Read Test"

start_time=`timestamp`
./concurrent_test.py .cmds $concurrency
end_time=`timestamp`

echo "Read Test Done"

elapsed=$(($end_time-$start_time))
echo "Elapsed[ms]=$elapsed"
echo "Bytes=$bytes"
bw=` bc <<< "scale=2; $bytes/$elapsed*1000/1024/1024"`
echo "BW[MB/s]=$bw"


# verify
error=0
for f in `cat .files` ; do
    fname=`basename $f`
    if [ `diff  $basedir/files_read/$fname  $f | wc -l` -gt 0 ]; then
        echo "Error: $f"
        let error=$error+1
    fi
done

if [ $error -gt 0 ] ; then
    exit 1
fi

umount /mount/tmp

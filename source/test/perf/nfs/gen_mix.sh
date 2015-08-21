#!/bin/bash

basedir=$1

if [ -d $basedir ]; then
    rm -f $basedir/*
else
    mkdir -p $basedir
fi

min_size=1
max_size=100

min_size_gb=1
max_size_gb=5
n=300
f=.1

sizes=`./gen_fsizes.py $min_size $max_size $min_size_gb $max_size_gb $n $f`
echo $sizes
for s in $sizes ; do
    echo "Generating $s"
    if [ -e $basedir/file_$s ]; then
        echo "renaming"
        mv $basedir/file_$s $basedir/file$s.$RANDOM
    fi
    ./gen_file.sh $s $basedir
done

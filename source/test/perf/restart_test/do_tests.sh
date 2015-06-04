#!/bin/bash

#sizes="256m 512m 1g 2g"
#sizes="5g"
sizes="64m 256m 512m 1g 2g 3g 4g 5g"
#sizes="256m 512m"
#sizes="64m"

for s in $sizes ; do
    outdir=results/test.$s\.$RANDOM
    mkdir $outdir
    ./run_test.sh $outdir /home/monchier/FDS/fds-src $s
done


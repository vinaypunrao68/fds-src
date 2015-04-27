#!/bin/bash

#sizes="256m 512m 1g 2g"
sizes="3g 4g 5g"

for s in $sizes ; do
    outdir=results/test.$s\.$RANDOM
    mkdir $outdir
    ./run_migration_test.sh $outdir /home/monchier/regress/fds-src $s
done


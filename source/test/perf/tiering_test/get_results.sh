#!/bin/bash
for f in `ls -d $1` ; do
    echo $f `grep iops $f | sed -e 's/[ ,=:]/ /g' | awk '{e+=$7}END{print e}'`
done


echo "Results"
for p in $policies ; do
    iops=`grep iops $outdir/out.$p | sed -e 's/[ ,=:]/ /g' | awk '{e+=$7}END{print e}'`
    frequency=`echo $outdir | awk -F "." '{print $2}'`
    echo $outdir $p $iops
    data="[
{
    "name" : "tiering",
    "columns" : ["iops", "outdir"],
    "points" : [[$iops, $outdir]]
  }
]"
    curl -X -d $data POST metrics.formationds.com/db/perf/tiering?u=perf&p=perf 
done

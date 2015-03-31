#!/bin/bash
for f in `ls -d $1` ; do
    #echo $f `grep iops $f | sed -e 's/[ ,=:]/ /g' | awk '{e+=$7}END{print e}'`
    iops=`grep iops $f | sed -e 's/[ ,=:]/ /g' | awk '{e+=$7}END{print e}'`
    frequency=`echo $f | sed -e 's/.*\.//g'`
    echo $f $p $iops
    data="[{\"name\" : \"tiering\",\"columns\" : [\"iops\", \"outdir\"],\"points\" : [[$iops, $f]]}]"
    # echo $data
    # curl -d $data -X POST metrics.formationds.com/db/perf/tiering?u=perf&p=perf 

done



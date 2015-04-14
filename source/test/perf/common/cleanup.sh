#!/bin/bash
cluster=$1
for m in $cluster ; do
    ssh $m "dpkg -l| grep fds| awk '{print \$2}' | xargs dpkg -P"
    ssh $m 'umount /fds/dev/*'
    ssh $m 'rm -fr /fds'
done


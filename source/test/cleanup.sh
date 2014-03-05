#!/bin/bash

# Run from the top level source dir
#
cleanup_bin_dir()
{
    pushd . > /dev/null
    cd $1
    echo "Cleanning up: `pwd`"

    rm -rf *.ldb 2>  /dev/null
    rm -rf logs stats 2> /dev/null
    rm -rf *SNode* 2> /dev/null
    rm -rf *ObjRankDB* 2> /dev/null
    rm om.*.log 2> /dev/null
    rm am.*.log 2> /dev/null
    rm sm.*.log 2> /dev/null
    rm -f ocache_stats.dat

    popd > /dev/null
}

cleanup_bin_dir "Build/linux-x86_64.debug/bin"
for f in `echo node2 node3 node4`; do
    bin_dir=../Build/linux-x86_64.debug/$f
    echo Cleaning up $bin_dir
    [ -d $bin_dir ] && cleanup_bin_dir $bin_dir
done
#cleanup_bin_dir "Build/linux-x86_64.debug/node2"
#cleanup_bin_dir "Build/linux-x86_64.debug/node3"
#cleanup_bin_dir "Build/linux-x86_64.debug/node4"

echo "Cleanning up data dir: /fds/*"
rm -rf /fds/hdd/sd?
rm -rf /fds/ssd/sd?
rm -rf /fds-node?/hdd/sd?
rm -rf /fds-node?/ssd/sd?
rm -rf /fds-node?/var/*
rm -rf /fds-am?/var/*
rm -rf /fds-node?/sys-repo/*
rm -rf /fds-node?/user-repo/*
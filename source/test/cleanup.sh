#!/bin/bash

##########################################################################################
# script to cleanup fds objects
##########################################################################################


# set the correct dirs
DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
FDSBINDIR="${DIR}/../Build/linux-x86_64.debug/bin"

function cleanBinDir() {
    pushd . > /dev/null
    cd $1
    echo "cleaning up: `pwd`"

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

function cleanFdsRoot() {
    echo "cleaning fds root /fds ..."
    rm -rf /fds/hdd/sd?
    rm -rf /fds/ssd/sd?
    rm -rf /fds/sys-repo/*
    rm -rf /fds/nginx/logs/*
    rm -rf /fds/user-repo/*
    rm -rf /fds-node?/hdd/sd?
    rm -rf /fds-node?/ssd/sd?
    rm -rf /fds-node?/var/*
    rm -rf /fds-am?/var/*
    rm -rf /fds-node?/sys-repo/*
    rm -rf /fds-node?/user-repo/*
    rm -rf /fds/node?/*
    
    # redis stuff
    echo "FLUSHALL" | redis-cli  >/dev/null 2>&1
    echo "BGREWRITEAOF" | redis-cli  >/dev/null 2>&1
    rm -rf $(find /fds/var/logs/* -not -name redis 2>/dev/null)
    rm -rf $(find /fds/var/* -not -name logs -not -name redis 2>/dev/null)
    rm -rf $(find /fds/logs/* -not -name redis 2>/dev/null) 
}

##########################################################################################
# Main module
##########################################################################################

cleanBinDir ${FDSBINDIR}

for node in node2 node3 node4; do
    bin_dir=../Build/linux-x86_64.debug/${node}
    [ -d $bin_dir ] && cleanBinDir ${bin_dir}

    # so we can run this from smoke test, at source dir
    bin_dir=./Build/linux-x86_64.debug/${node}
    [ -d $bin_dir ] && cleanBinDir ${bin_dir}
done

cleanFdsRoot

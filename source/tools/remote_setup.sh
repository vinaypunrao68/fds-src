#!/bin/bash

main_node_ip=$1


pushd /usr/local/lib/python2.7/dist-packages/
    tar xzvf /tmp/fdslib.tgz --strip-component=1
popd

pushd /fds/
    if [ -e bin ]; then
        rm -fr bin
    fi
    
    if [ -e sbin ]; then
        rm -fr sbin
    fi

    if [ -e core ]; then
        rm -fr core
    fi
    
    if [ -e etc ]; then
        rm -fr etc
    fi

    if [ -e lib ]; then
        rm -fr lib
    fi

    if [ -e tools ]; then
        rm -fr tools
    fi

    if [ ! -e sbin ]; then
        mkdir sbin
    fi
    
    if [ ! -e core ]; then
        mkdir core
    fi
    
    tar xzvf /tmp/fds-package.tgz --strip-components=2
    tar xzvf /tmp/fds-config.tgz --strip-components=1
    
    pushd sbin
            tar xzvf /tmp/fds-tools.tgz
            pushd tools/install
                ./fdsinstall.py -i -o 3
                ./fdsinstall.py -i -o 4
                ./disk_type.py -m
            popd
            mv test/fds-tool.py .
            rmdir test
            if [ ! -e redis.sh ]; then
                ln -s tools/redis.sh
            fi
            if [ -e ../formation.conf ]; then
                mv ../formation.conf .
            fi
    popd
    pushd etc
            sed "s/om_ip.*/om_ip = \"$main_node_ip\"/" platform.conf > platform.conf.tmp
            sed "s/am_host.*/am_host = \"$main_node_ip\"/" platform.conf.tmp > platform.conf
            sed "s/om_ip.*/om_ip = \"$main_node_ip\"/" orch_mgr.conf > orch_mgr.conf.tmp
            sed "s/ip_address.*/ip_address = \"$main_node_ip\"/" orch_mgr.conf.tmp > orch_mgr.conf
    popd
popd

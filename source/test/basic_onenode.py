#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import subprocess
import logging
from fdslib.FdsCluster import FdsCluster
from fdslib import process
import optparse, sys, time


if __name__ == '__main__':
    process.setup_logger()

    (options, args) = process.parse_fdscluster_args()

    cluster = FdsCluster(options.config_file)
    time.sleep(5)

    cluster.add_node('node1', ['sm', 'dm'])

    cluster.get_service('node1', 'sm').wait_for_healthy_state()

    cluster.start_ams()
    time.sleep(5)

    # run some io
    io_proc = subprocess.Popen(['/home/nbayyana/bin/uploads3.sh'])
    
    time.sleep(2)

    # add one more node
    cluster.add_node('node2', ['sm'])

    cluster.get_service('node2', 'sm').wait_for_healthy_state()

    io_proc.wait()

    # run checker
    # cluster.run_dirbased_checker('/home/nbayyana/temp/skinet2')
    p.wait()

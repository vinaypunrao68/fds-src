#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import sys
import os
sys.path.append(os.getcwd()+'/fdslib/pyfdsp')
import subprocess
import logging
from fdslib.FdsCluster import FdsCluster
from fdslib import process
import optparse, sys, time
from fdslib import IOGen

if __name__ == '__main__':
    process.setup_logger()
    (options, args) = process.parse_fdscluster_args()

    cluster = FdsCluster(options.config_file)
    time.sleep(5)

    cluster.add_node('node1', ['sm', 'dm'])

    cluster.get_service('node1', 'sm').wait_for_healthy_state(timeout_s=360)

    cluster.start_ams()
    time.sleep(5)

    # run some io
    IOGen.PutGenerator(endpoint = '{}:8000'.format(cluster.get_node('node1').get_ip()),
                 bucket = 'b1',
                 dir = '/home/nbayyana/temp/skinet2',
                 repeat_cnt=1).run()

    # add node2
    cluster.add_node('node2', ['sm'])
    cluster.get_service('node2', 'sm').wait_for_healthy_state(timeout_s=360)

    # add node3
    cluster.add_node('node3', ['sm'])
    cluster.get_service('node3', 'sm').wait_for_healthy_state(timeout_s=360)

    # run checker
    cluster.run_dirbased_checker('/home/nbayyana/temp/skinet2')

    sys.exit(0)
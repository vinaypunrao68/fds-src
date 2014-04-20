#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import subprocess
import logging
from fdslib.FdsCluster import FdsCluster
from fdslib import ProcessUtils
import optparse, sys, time


if __name__ == '__main__':
    ProcessUtils.setup_logger()

    (options, args) = ProcessUtils.parse_fdscluster_args()

    cluster = FdsCluster(options.config_file)
    time.sleep(5)

    cluster.add_node('node1', ['sm', 'dm'])

    cluster.get_service('node1', 'sm').wait_for_healthy_state()

    cluster.start_ams()
    time.sleep(5)

    # run some io
    # (TODO: Rao) Change to some constant io
    io_proc = subprocess.Popen(['/home/nbayyana/bin/uploads3.sh'])
    
    time.sleep(2)

    # add one more node
    cluster.add_node('node2', ['sm'])
    cluster.get_service('node2', 'sm').wait_for_healthy_state()

    # run checker
    cluster.run_dirbased_checker('/home/nbayyana/temp/skinet2')

    # # add one more node
    # cluster.add_node('node3', ['sm'])
    # cluster.get_service('node3', 'sm').wait_for_healthy_state()
    #
    # # add one more node
    # cluster.add_node('node4', ['sm'])
    # cluster.get_service('node4', 'sm').wait_for_healthy_state()
    #
    # # wait about a minute for io to go one
    # time.sleep(60)

    cluster.remove_node('node2')

    # wait for some time
    time.sleep(20)

    # kill the process
    io_proc.kill()

    # todo(Rao): Make sure all services are alive
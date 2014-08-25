#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import sys
import os
sys.path.append(os.getcwd()+'/fdslib/pyfdsp')


import sys
import time
from multiprocessing import Process

from fdslib.FdsCluster import FdsCluster
from fdslib import process
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

    get_gen = IOGen.GetGenerator(endpoint = '{}:8000'.format(cluster.get_node('node1').get_ip()),
                                 bucket = 'b1',
                                 dir = '/home/nbayyana/temp/skinet2',
                                 repeat_cnt=20,
                                 sleep=1)
    get_proc = Process(target=get_gen.run)
    get_proc.start()

    # run some io
    # (TODO: Rao) Change to some constant io
    # io_proc = subprocess.Popen(['/home/nbayyana/bin/uploads3.sh'])
        # (TODO: Rao) Change to some constant io
    # io_proc = subprocess.Popen(['python', 'fds-primitive-smoke.py', '--up', 'false',
    #                             '--down', 'false',
    #                             '--smoke_test', 'true',
    #                             '--put_only', 'true',
    #                             '--thread', '1',
    #                             '--am_ip', '127.0.0.1',
    #                             '--data_set', '/home/nbayyana/temp/skinet2'])
    # if io_proc.wait() is not 0:
    #     raise Exception('fds-primitive-smoke failed')
    
    time.sleep(2)

    # add node2
    cluster.add_node('node2', ['sm'])
    cluster.get_service('node2', 'sm').wait_for_healthy_state(timeout_s=360)

    # add node3
    cluster.add_node('node3', ['sm'])
    cluster.get_service('node3', 'sm').wait_for_healthy_state(timeout_s=360)

    # # add node4
    # cluster.add_node('node4', ['sm'])
    # cluster.get_service('node4', 'sm').wait_for_healthy_state(timeout_s=360)

    # run checker
    cluster.run_dirbased_checker('/home/nbayyana/temp/skinet2')

    # remove a node
    cluster.remove_node('node2')

    # wait for some time
    time.sleep(10)

    # run checker
    cluster.run_dirbased_checker('/home/nbayyana/temp/skinet2')

    # kill the process
    get_proc.terminate()

    # todo(Rao): Make sure all services are alive

    sys.exit(0)
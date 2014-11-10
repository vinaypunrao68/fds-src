#!/usr/bin/env python

import fdslib.FdsSetup as inst
import fdslib.BringUpCfg as fdscfg
import fdslib.BasicCluster as BasicCluster
import optparse
import time
import threading
import os
import pdb
import multiprocessing
import signal
import sys

# To run this test, following steps are required
# 
# To all nodes:
#   1) disable authentication in platform.conf
#   2) copy sm_ut.conf from source/unit-test/stor-mgr
#   3) copy over source/test/traffic_gen.py to either root directory or /fds/sbin
#      location of traffic_agen.py is hard-coded currently.
#
# Known issues:
#   1) platform crashes if restarted.  must clean up FDS before restarting.  unfortunately
#      we lose on-disk data, but once fixed, we can update the test.

def setup_system(cluster):
    print "Bringing down fds"
    cluster.exec_cmd_am_nodes("/fds/sbin/fds-tool.py -d -f /fds/sbin/formation.conf")

    print "Cleaning up fds"
    cluster.exec_cmd_am_nodes("/fds/sbin/fds-tool.py -c -f /fds/sbin/formation.conf")

    print "Bringing up fds"
    cluster.exec_cmd_am_nodes("/fds/sbin/fds-tool.py -u -f /fds/sbin/formation.conf")

def generate_load(cluster):
    print "Generating load on systems"
    cluster.exec_cmd_am_nodes("curl -v -X PUT http://localhost:8000/volume0", True)
    cluster.exec_cmd_am_nodes("/root/test/traffic_gen.py -t 10 -n 10000 -T PUT -s 4096 -F 1000 -v 1 -u", True)

def shutdown_daemons_on_all_nodes(cluster):
    print "Terminating FDS"
    cluster.shutdown_daemons()

def check_sm_integrity(cluster):
    print "Checking SM integrity"
    cluster.exec_cmd_all_nodes("/fds/bin/smchk", True)

def shutdown_with_load_test(cluster):
    setup_system(cluster)
    
    print "Current FDS cluster status"
    cluster.status(True)
    time.sleep(5)

    print "Creating timer to terminate all FDS processes in 75 seconds"
    term_thread = threading.Timer(75, shutdown_daemons_on_all_nodes, [cluster])
    term_thread.start()

    print "Generating load"
    load_thread = threading.Thread(target = generate_load, args = [cluster])
    load_thread.start()

    term_thread.join()

    cluster.status(True)

    check_sm_integrity(cluster)

    os.kill(os.getpid(), signal.SIGKILL)

def main():
    print "Terminating cluster"

    parser = optparse.OptionParser("usage: %prog [options]")

    parser.add_option('-f', '--file', dest = 'config_file', default = 'formation.conf',
                      help = 'configuration file (e.g. sample.cfg)', metavar = 'FILE')
    parser.add_option('-v', '--verbose', action = 'store_true', dest = 'verbose',
                      help = 'enable verbosity')
    parser.add_option('-R', '--fds-root', dest = 'fds_root', default = '/fds',
                      help = 'specify fds-root directory')
    parser.add_option('-r', '--dryrun', action = 'store_true', dest = 'dryrun',
                      help = 'dry run, print commands only')

    (options, args) = parser.parse_args()

    env = inst.FdsEnv('.');

    if options.config_file is None:
        print "Config file is required"
        sys.exit(0)

    print "Using config file", options.config_file

    config = fdscfg.FdsConfigRun (env, options)

    cluster = BasicCluster.basic_cluster (config.rt_get_obj('cfg_nodes'), 
                                          config.rt_get_obj('cfg_am'), 
                                          config.rt_get_obj('cfg_cli'))
    
    shutdown_with_load_test(cluster)

if __name__ == '__main__':
    main()

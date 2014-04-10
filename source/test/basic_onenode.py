#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
from fdslib.FdsCluster import FdsCluster
from fdslib import ProcessUtils
import optparse, sys, time


if __name__ == '__main__':
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option('-f', '--file', dest = 'config_file',
                      help = 'configuration file (e.g. sample.cfg)', metavar = 'FILE')
    parser.add_option('-v', '--verbose', action = 'store_true', dest = 'verbose',
                      help = 'enable verbosity')

    (options, args) = parser.parse_args()


    if options.config_file is None:
        print "You need to pass config file"
        sys.exit(1)
    
    cluster = FdsCluster(options.config_file)
    time.sleep(5)
    cluster.start_ams()
    time.sleep(5)
    cluster.add_node('node1', ['sm', 'dm'])
    time.sleep(10)
    # run some io
    ProcessUtils.run('/home/nbayyana/bin/uploads3.sh', wait_time_sec = 120)
    
    time.sleep(2)

    # add one more node
    cluster.add_node('node2', ['sm'])

    # run checker
    cluster.run_dirbased_checker('/home/nbayyana/temp/skinet2')
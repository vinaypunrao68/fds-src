#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import fdslib.FdsSetup as inst
import fdslib.BringUpCfg as fdscfg
import optparse, sys, time
import pdb

if __name__ == '__main__':
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option('-f', '--file', dest = 'config_file',
                      help = 'configuration file (e.g. sample.cfg)', metavar = 'FILE')
    parser.add_option('-v', '--verbose', action = 'store_true', dest = 'verbose',
                      help = 'enable verbosity')
    parser.add_option('-u', '--up', action = 'store_true', dest = 'clus_up',
                      help = 'bring up cluster')
    parser.add_option('-d', '--down', action = 'store_true', dest = 'clus_down',
                      help = 'bring down cluster')
    parser.add_option('-c', '--clean', action = 'store_true', dest = 'clus_clean',
                      help = 'cleanup cluster')
    parser.add_option('-r', '--dryrun', action = 'store_true', dest = 'dryrun',
                      help = 'dry run, print commands only')
    parser.add_option('-R', '--fds-root', dest = 'fds_root', default = '/fds',
                      help = 'Specify fds-root directory')

    (options, args) = parser.parse_args()

    env = inst.FdsEnv(options.fds_root)
    if options.config_file is None:
        print "You need to pass config file"
        sys.exit(1)

    cfg = fdscfg.FdsConfigRun(env, options.config_file, options.verbose)

    # Get all the configuration
    nodes = cfg.rt_get_obj('cfg_nodes')
    ams   = cfg.rt_get_obj('cfg_am')
    steps = cfg.rt_get_obj('cfg_scenarios')

    # Shutdown
    if options.clus_down:
        for n in nodes:
            n.nd_cleanup_daemons()

    # Cleanup
    if options.clus_clean:
        for n in nodes:
            n.nd_cleanup_node()

    cfg.rt_fds_bootstrap()

    if options.clus_up is None:
        sys.exit(0)


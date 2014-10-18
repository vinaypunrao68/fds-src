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
    parser.add_option('-p', '--package', action = 'store_true', dest = 'make_pkg',
                      help = 'package fds tar ball')
    parser.add_option('-i', '--install', action = 'store_true', dest = 'tar_file',
                      help = 'install fds package to remote machines')
    parser.add_option('-m', '--manual', action = 'store_true', dest = 'manual',
                      help = 'Only run OM and platform, admit nodes manually')
    parser.add_option('-U', '--UP', action = 'store_true', dest = 'platf_up',
                      help = 'bring up platform only')
    parser.add_option('-u', '--up', action = 'store_true', dest = 'clus_up',
                      help = 'bring up cluster')
    parser.add_option('-d', '--down', action = 'store_true', dest = 'clus_down',
                      help = 'bring down cluster')
    parser.add_option('-c', '--clean', action = 'store_true', dest = 'clus_clean',
                      help = 'cleanup cluster')
    parser.add_option('-r', '--dryrun', action = 'store_true', dest = 'dryrun',
                      help = 'dry run, print commands only')

    (options, args) = parser.parse_args()

    # It's not important that we get fds-root here.
    env = inst.FdsEnv('.')
    pkg = inst.FdsPackage(env)
    if options.make_pkg:
        print "Make fds package from ", env.get_fds_source()
        pkg.package_tar()
        if options.config_file is None:
            sys.exit(0)

    # Get all the objects
    cfg   = fdscfg.FdsConfigRun(env, options)
    nodes = cfg.rt_get_obj('cfg_nodes')
    ams   = cfg.rt_get_obj('cfg_am')
    pols  = cfg.rt_get_obj('cfg_vol_pol')
    vols  = cfg.rt_get_obj('cfg_volumes')
    steps = cfg.rt_get_obj('cfg_scenarios')

    # Install package
    if options.tar_file:
        for n in nodes:
            n.nd_install_rmt_pkg()

    # shutdown
    if options.clus_down:
        for n in nodes:
            n.nd_cleanup_daemons_with_fdsroot(fds_root='')

    # cleanup
    if options.clus_clean:
        for n in nodes:
            n.nd_cleanup_node()

    if options.clus_up is None:
        sys.exit(0)

    if options.platf_up:
        for n in nodes:
            n.nd_start_platform('localhost')
        sys.exit(0)

    # If we have scenarios to run steps, run them
    if len(steps):
        for sce in steps:
            sce.start_scenario()
        sys.exit(0)

    # else run all defined sections by default
    time.sleep(2)
    cfg.rt_fds_bootstrap()

    if options.manual:
        print('You need to run fdscli --activate-nodes manually.')
        sys.exit(0)

    time.sleep(5)
    cli = cfg.rt_get_obj('cfg_cli')
    cli.run_cli('--activate-nodes abc -k 1 -e sm,dm')

    for am in ams:
        am.am_start_service()

    time.sleep(5)
    for p in pols:
        p.policy_apply_cfg(cli)

    for v in vols:
        v.vol_apply_cfg(cli)

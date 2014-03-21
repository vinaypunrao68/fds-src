#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import fdslib.FdsSetup as inst
import fdslib.BringUpCfg as fdscfg
import optparse, sys, time

if __name__ == '__main__':
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option('-f', '--file', dest = 'config_file',
                      help = 'configuration file (e.g. sample.cfg)', metavar = 'FILE')
    parser.add_option('-v', '--verbose', action = 'store_true', dest = 'verbose',
                      help = 'enable verbosity')
    parser.add_option('-p', '--package', action = 'store_true', dest = 'make_pkg',
                      help = 'package fds tar ball')
    parser.add_option('-i', '--install', action = 'store_true', dest = 'clus_inst',
                      help = 'install fds package to remote machines')
    parser.add_option('-m', '--manual', action = 'store_true', dest = 'manual',
                      help = 'Only run OM and platform, admit nodes manually')
    parser.add_option('-u', '--up', action = 'store_true', dest = 'clus_up',
                      help = 'bring up cluster')
    parser.add_option('-d', '--down', action = 'store_true', dest = 'clus_down',
                      help = 'bring down cluster')

    (options, args) = parser.parse_args()

    # It's not important that we get fds-root here.
    env = inst.FdsEnv('.')
    pkg = inst.FdsPackage(env)
    if options.make_pkg:
        print "Make fds package from ", env.get_fds_source()
        pkg.package_tar()
        if options.config_file is None:
            sys.exit(0)

    if options.config_file is None:
        print "You need to pass config file"
        sys.exit(1)

    cfg = fdscfg.FdsConfig(options.config_file, options.verbose)
    cfg.config_parse()
    user = cfg.config_user()
    if user is not None:
        usr = user[0]
        env.env_user     = usr.get_config_val('user_name')
        env.env_password = usr.get_config_val('password')

    nodes = cfg.config_nodes()
    for n in nodes:
        n.nd_connect_rmt_agent(env)
        n.nd_rmt_agent.ssh_setup_env('')
        if options.clus_inst:
            n.nd_install_rmt_pkg() 

    if options.clus_down:
        for n in nodes:
            n.nd_cleanup_daemons()

    if options.clus_up is None:
        sys.exit(0)

    time.sleep(2)
    for n in nodes:
        if n.nd_start_om() != 0:
            break

    for n in nodes:
        n.nd_start_platform()

    time.sleep(5)
    cli = cfg.config_cli()
    cli.run_cli('--activate-nodes abc -k 1 -e sm,dm')

    if options.manual:
        print('You need to run fdscli --activate-nodes manually.')
        sys.exit(0)

    for am in cfg.config_am():
        am.am_start_service()

    time.sleep(5)
    pols = cfg.config_vol_policy()
    for p in pols:
        p.policy_apply_cfg(cli)

    vols = cfg.config_volumes()
    for v in vols:
        v.vol_apply_cfg(cli)

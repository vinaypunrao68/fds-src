#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import optparse, sys, time
sys.path.append("/fds/lib/python2.7/dist-packages")
import fdslib.FdsSetup as inst
import fdslib.BringUpCfg as fdscfg
import os
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
    parser.add_option('-S', '--source', dest = 'fds_source_dir', default = '',
                      help = 'fds source dir')
    parser.add_option('-u', '--up', action = 'store_true', dest = 'clus_up',
                      help = 'bring up cluster')
    parser.add_option('--reboot', action = 'store_true', dest = 'reboot',
                      help = 'reboot cluster')
    parser.add_option('-d', '--down', action = 'store_true', dest = 'clus_down',
                      help = 'bring down cluster')
    parser.add_option('-s', '--status', action = 'store_true', dest = 'clus_status',
                      help = 'get cluster status')
    parser.add_option('-c', '--clean', action = 'store_true', dest = 'clus_clean',
                      help = 'cleanup cluster')
    parser.add_option('-r', '--dryrun', action = 'store_true', dest = 'dryrun',
                      help = 'dry run, print commands only')
    parser.add_option('-R', '--fds-root', dest = 'fds_root', default = '/fds',
                      help = 'specify fds-root directory')
    parser.add_option('-t', '--target', dest = 'target', default = None,
                      help = 'target node to apply -u, -d, -c, and -s against')

    (options, args) = parser.parse_args()

    if options.config_file is None:
        print "Unless you're just packaging, you need to pass a config file!"
        #sys.exit(1)

    # Packaging
    env = inst.FdsEnv('.')
    pkg = inst.FdsPackage(env)
    if options.make_pkg:
        print "Make fds package from ", env.get_fds_source()
        pkg.package_tar()
        if options.config_file is None:
            sys.exit(0)

    cfg = fdscfg.FdsConfigRun(env, options)

    # Get all the configuration
    nodes = cfg.rt_get_obj('cfg_nodes')
    ams   = cfg.rt_get_obj('cfg_am')
    cli   = cfg.rt_get_obj('cfg_cli')
    steps = cfg.rt_get_obj('cfg_scenarios')
    start_om = True

    # Install package
    if options.tar_file:
        for n in nodes:
            n.nd_install_rmt_pkg()
        exit(0)

    # filter nodes and ams based on the target
    if options.target:
        print "Target specified.  Applying commands against target: {}".format(options.target)
        # filter nodes
        nodes = [n for n in nodes if n.nd_conf_dict['node-name'] == options.target]
        if len(nodes) is 0:
            print "No matching nodes for the target"
            sys.exit(1)
        # filter ams
        ams = [a for a in ams if a.nd_conf_dict['fds_node'] == options.target]
        # Check if OM needs to be started
        if cfg.rt_om_node.nd_conf_dict['node-name'] != options.target:
            start_om = False


    # Shutdown
    if options.clus_down:
        for n in nodes:
            n.nd_cleanup_daemons()
            # todo(Rao): Unregister the services here.  Once we implement name based
            # services this should be easy
            print("NOTE: Run fdscli --remove-services to remove services")

    # Cleanup
    if options.clus_clean:
        for n in nodes:
            n.nd_cleanup_node()
            n.nd_agent.ssh_exec('(cd {} && ./redis.sh clean)'.format(
                os.path.join(options.fds_root, 'sbin')), output=True)
            n.nd_agent.ssh_exec('rm {}'.format(
                os.path.join(options.fds_root, 'uuid_port')))

    # Status
    if options.clus_status:
        print "============================================================================================="
        print "= fds-tool.py -s --status is deprecated, please use fdsadmin --status (or --process-status) ="
        print "============================================================================================="
     
        for n in nodes:
            n.nd_agent.ssh_exec('ps -ef | grep -v grep | grep -v bash | grep com.formationds.om.Main', output = True)
            n.nd_agent.ssh_exec('ps -ef | grep -v grep | grep -v bash | grep com.formationds.am.Main', output = True)
            n.nd_agent.ssh_exec('ps -ef | grep -v grep | grep -v bash | grep bare_am', output = True)
            n.nd_agent.ssh_exec('ps -ef | grep -v grep | grep -v bash | grep platformd', output = True)
            n.nd_agent.ssh_exec('ps -ef | grep -v grep | grep -v bash | grep Mgr', output = True)
            print '\n'
        sys.exit(0)

    if options.reboot:
        om = cfg.rt_om_node
        om_ip = om.nd_conf_dict['ip']

        for n in nodes:
            n.nd_start_platform(om_ip)

        if start_om:
            print "Start OM on IP", om_ip
            om.nd_start_om()
        time.sleep(30) # Do not remove this sleep
        sys.exit(0)

    if options.clus_up is None:
        sys.exit(0)

    for n in nodes:
        n.nd_agent.ssh_exec('python -m disk_type -m', wait_compl=True)
        n.nd_agent.ssh_exec('{} start'.format(
            os.path.join(options.fds_root, 'sbin/redis.sh')), wait_compl=True, output=True)

    om = cfg.rt_om_node
    om_ip = om.nd_conf_dict['ip']

    for n in nodes:
        n.nd_start_platform(om_ip)

    if start_om:
        print "Start OM on IP", om_ip
        om.nd_start_om()
	time.sleep(8)

    cli.run_cli('--activate-nodes abc -k 1 -e sm,dm,am')
    print "Waiting for services to come up"
    time.sleep(8)

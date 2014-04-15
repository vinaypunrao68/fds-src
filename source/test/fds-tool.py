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
    parser.add_option('-s', '--status', action = 'store_true', dest = 'clus_status',
                      help = 'get cluster status')
    parser.add_option('-c', '--clean', action = 'store_true', dest = 'clus_clean',
                      help = 'cleanup cluster')
    parser.add_option('-r', '--dryrun', action = 'store_true', dest = 'dryrun',
                      help = 'dry run, print commands only')
    parser.add_option('-R', '--fds-root', dest = 'fds_root', default = '/fds',
                      help = 'specify fds-root directory')

    (options, args) = parser.parse_args()

    if options.config_file is None:
        print "You need to pass config file"
        sys.exit(1)

    cfg = fdscfg.FdsConfigRun(None, options)

    # Get all the configuration
    nodes = cfg.rt_get_obj('cfg_nodes')
    ams   = cfg.rt_get_obj('cfg_am')

    # Shutdown
    if options.clus_down:
        for n in nodes:
            n.nd_cleanup_daemons()

    # Cleanup
    if options.clus_clean:
        for n in nodes:
            n.nd_cleanup_node()
            n.nd_rmt_agent.ssh_exec('/fds/sbin/redis.sh clean')
       
    # Status
    if options.clus_status:
        for n in nodes:
            n.nd_rmt_agent.ssh_exec('ps -ef | grep -v grep | grep -v base | grep com.formationds.web.om.Main', output = True)
            n.nd_rmt_agent.ssh_exec('ps -ef | grep -v grep | grep -v bash | grep plat', output = True)
            n.nd_rmt_agent.ssh_exec('ps -ef | grep -v grep | grep -v bash | grep Mgr', output = True)
            n.nd_rmt_agent.ssh_exec('ps -ef | grep -v grep | grep -v bash | grep AMA', output = True)
            print '\n'
        sys.exit(0)

    if options.clus_up is None:
        sys.exit(0)

    for n in nodes:
        n.nd_rmt_agent.ssh_exec('python -m disk_type -m')
        n.nd_rmt_agent.ssh_exec('/fds/sbin/redis.sh start')

    om = cfg.rt_om_node
    om_ip = om.nd_conf_dict['ip']
    print "Start OM on IP", om_ip

    om.nd_start_om()
    time.sleep(2)

    cli = cfg.rt_get_obj('cfg_cli')
    for n in nodes:
        n.nd_start_platform(om_ip)
        if n.nd_conf_dict['node-name'] == 'node1':
            cli.run_cli('--activate-nodes abc -k 1 -e sm,dm')
        else:
            cli.run_cli('--activate-nodes abc -k 1 -e sm')
        print "Waiting for node %s to come up" % n.nd_rmt_host
    	time.sleep(3)

    time.sleep(4)
    for am in ams:
        am.am_start_service()

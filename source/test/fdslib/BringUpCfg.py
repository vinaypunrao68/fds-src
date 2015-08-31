#!/usr/bin/env python

# Copyright 2014 by Formations Data Systems, Inc.
#
import ConfigParser
import re
import sys
import subprocess
import time
import logging
import os
import os.path
import FdsSetup as inst
import socket
import requests
from requests.adapters import HTTPAdapter
import TestUtils
###
# Base config class, which is key/value dictionary.
#
class FdsConfig(object):
    def __init__(self, items, cmd_line_options):
        self.nd_cmd_line_options   = cmd_line_options
        self.nd_conf_dict = {}
        if items is not None:
            for it in items:
                self.nd_conf_dict[it[0]] = it[1]

    def get_config_val(self, key):
        return self.nd_conf_dict[key]

    def debug_print(self, mesg):
        if self.nd_cmd_line_options['verbose'] or self.nd_cmd_line_options['dryrun']:
            print mesg
        return self.nd_cmd_line_options['dryrun']

###
# Handle node config section
#
class FdsNodeConfig(FdsConfig):
    def __init__(self, name, items, cmd_line_options, nodeId = 0, rt_env = None):
        log = logging.getLogger(self.__class__.__name__ + '.' + '__init__')

        super(FdsNodeConfig, self).__init__(items, cmd_line_options)
        self.nd_conf_dict['node-name'] = name
        self.nd_host  = None
        self.nd_agent = None
        self.nd_package   = None
        self.nd_local_env = None
        self.nd_assigned_name = None
        self.nd_uuid = None
        self.nd_services = None
        self.rt_env = rt_env

        # Is the node local or remote?
        if 'ip' not in self.nd_conf_dict:
            log.error("Missing ip keyword in the node section %s." % self.nd_conf_dict['node-name'])
            sys.exit(1)
        else:
            ipad = socket.gethostbyname(self.nd_conf_dict['ip'])
            if ipad.count('.') == 4:
                hostName = socket.gethostbyaddr(ipad)[0]
            else:
                hostName = self.nd_conf_dict['ip']

            if (cmd_line_options['install'] == False or cmd_line_options['install'] is None) and ((hostName == 'localhost') or (ipad == '127.0.0.1') or (ipad == '127.0.1.1')):
                self.nd_local = True

            elif cmd_line_options['install']== True: #it's definately remote env as command line argument was passed
                # With a remote installation we will assume a package install using Ansible.
                self.nd_local = False

                if 'inventory_file' not in cmd_line_options:
                    log.error("Need to provide inventory file name to run tests against deployed nodes")

                ips_array = TestUtils.get_ips_from_inventory(cmd_line_options['inventory_file'],rt_env)
                if (ips_array.__len__() < (nodeId+1)):
                    raise Exception ("Number of ips give in inventory are less than nodes in cfg file")

                if 'om' in self.nd_conf_dict:
                    #TODO Pooja: do more correctly, currently assuming that first ip in list is OM IP
                    self.nd_conf_dict['ip'] = ips_array[nodeId]
                else:
                    self.nd_conf_dict['ip'] = ips_array[nodeId]

                # In this case, the deployment scripts always sets "/fds" as fds_root
                # regardless of test configuration.
                self.nd_conf_dict['fds_root'] = '/fds'

                # Additionally, the deployment scripts always sets the node's base port as 7000
                # regardless of test configuration.
                self.nd_conf_dict['fds_port'] = '7000'

                # Additionally, we currently always need to boot Redis for a non-local node.
                self.nd_conf_dict['redis'] = 'true'

    ###
    # Establish ssh connection with the remote node.  After this call, the obj
    # can use nd_agent to send ssh commands to the remote node.
    #
    # If the IP indicates the node is localhost and we're running from the
    # Test Harness, no SSH connection is made.
    #
    def nd_connect_agent(self, env, quiet=False):
        log = logging.getLogger(self.__class__.__name__ + '.' + 'nd_connect_agent')

        if 'fds_root' not in self.nd_conf_dict:
            if env.env_test_harness:
                log.error("Missing fds-root keyword in the node section %s." % self.nd_conf_dict['node-name'])
            else:
                print("Missing fds-root keyword in the node section %s." % self.nd_conf_dict['node-name'])
            sys.exit(1)

        if 'ip' not in self.nd_conf_dict:
            if env.env_test_harness:
                log.error("Missing ip keyword in the node section %s." % self.nd_conf_dict['node-name'])
            else:
                print("Missing ip keyword in the node section %s." % self.nd_conf_dict['node-name'])
            sys.exit(1)

        self.nd_local_env = env
        self.nd_host  = self.nd_conf_dict['ip']

        root = self.nd_conf_dict['fds_root']

        # Create the "node agent", the object used to execute shell commands on the node.
        if env.env_test_harness:
            if self.nd_local:
                self.nd_agent = inst.FdsLocalEnv(root, verbose=self.nd_cmd_line_options, install=env.env_install,
                                                 test_harness=env.env_test_harness)
            else:
                self.nd_agent = inst.FdsRmtEnv(root, verbose=self.nd_cmd_line_options, test_harness=env.env_test_harness)

            # Set the default user ID from the Environment.
            self.nd_agent.env_user = env.env_user
            self.nd_agent.env_password = env.env_password
            self.nd_agent.env_sudo_password = env.env_sudo_password

            # Pick up anything configured for the node.
            if 'user_name' in self.nd_conf_dict:
                self.nd_agent.env_user = self.nd_conf_dict['user_name']

            if 'password' in self.nd_conf_dict:
                self.nd_agent.env_password = self.nd_conf_dict['password']

            if 'sudo_password' in self.nd_conf_dict:
                self.nd_agent.env_sudo_password = self.nd_conf_dict['sudo_password']
        else:
            self.nd_agent = inst.FdsRmtEnv(root, verbose=self.nd_cmd_line_options, test_harness=env.env_test_harness)

            self.nd_agent.env_user = env.env_user
            self.nd_agent.env_password = env.env_password

        # At this time, only Test Harness usage will treat localhost
        # connectivity locally. This is to satisfy Jenkins/Docker
        # usages. Otherwise, all hosts, localhost or otherwise, are
        # treated remotely with SSH connectivity.
        if self.nd_local and env.env_test_harness:
            if not quiet:
                log.info("Making local connection to %s/%s as user %s." %
                         (self.nd_host, self.nd_conf_dict['node-name'], self.nd_agent.env_user))

            self.nd_agent.local_connect(user=self.nd_agent.env_user, passwd=self.nd_agent.env_password,
                                        sudo_passwd=self.nd_agent.env_sudo_password)
        else:
            if not quiet:
                if env.env_test_harness:
                    log.info("Making remote ssh connection to %s/%s as user %s." %
                             (self.nd_host, self.nd_conf_dict['node-name'], self.nd_agent.env_user))
                else:
                    print("Making ssh connection to %s as %s." % (self.nd_host, self.nd_conf_dict['node-name']))

            if env.env_test_harness:
                self.nd_agent.ssh_connect(self.nd_host, user=self.nd_agent.env_user, passwd=self.nd_agent.env_password)
            else:
                self.nd_agent.ssh_connect(self.nd_host)

    ###
    # Install the tar ball package at local location to the remote node.
    #
    def nd_install_rmt_pkg(self):
        if self.nd_agent is None:
            print "You need to call nd_connect_agent() first"
            sys.exit(0)

        print "Installing FDS package to:", self.nd_host_name()
        pkg = inst.FdsPackage(self.nd_agent)
        tar = self.nd_local_env.get_pkg_tar()
        status = pkg.package_install(self.nd_agent, tar)

        return status

    def nd_host_name(self):
        return '[' + self.nd_host + '] ' + self.nd_conf_dict['node-name']

    ###
    # Return True if this node is configured to run OM.
    #
    def nd_run_om(self):
        if 'om' in self.nd_conf_dict:
            return True if self.nd_conf_dict['om'] == "true" else False
        return False

    ###
    # We only start OM if the node is configured to run OM.
    #
    def nd_start_om(self, test_harness=False, _bin_dir=None, _log_dir=None):
        log = logging.getLogger(self.__class__.__name__ + '.' + __name__)

        if self.nd_run_om():
            om_ip_arg = ' --fds.common.om_ip_list=%s' % self.nd_conf_dict['ip']
            if self.nd_agent is None:
                print "You need to call nd_connect_agent() first"
                sys.exit(0)

            fds_dir = self.nd_conf_dict['fds_root']

            if _bin_dir is None:
                bin_dir = fds_dir + '/bin'
            else:
                bin_dir = _bin_dir

            if _log_dir is None:
                log_dir = bin_dir
            else:
                log_dir = _log_dir

            print "Start OM on %s in %s" % (self.nd_host_name(),fds_dir)

            self.nd_start_influxdb();

            if test_harness:
                status = self.nd_agent.exec_wait('bash -c \"(nohup ./orchMgr --fds-root=%s %s > %s/om.out 2>&1 &) \"' %
                                                 (fds_dir, om_ip_arg, log_dir),
                                                 fds_bin=True)
            else:
                status = self.nd_agent.ssh_exec_fds(
                    "orchMgr --fds-root=%s %s > %s/om.out" % (fds_dir, om_ip_arg, log_dir))

            time.sleep(2)
        else:
            log.warning("Attempting to start OM on node %s which is not configured to host OM." % self.nd_host_name())
            status = -1

        return status

    def nd_start_influxdb(self):
        log = logging.getLogger(self.__class__.__name__ + '.' + __name__)
        log.info("Starting InfluxDB on %s" % (self.nd_host_name()))

        ## check if influx is running on the node.  If not, start it
        influxstat = self.nd_status_influxdb()
        if influxstat != 0:
            status = self.nd_agent.exec_wait('service influxdb start 2>&1 >> /dev/null', output=False)
            if status != 0:
                print "Failed to start InfluxDB on %s" % (self.nd_host_name())
                log.info("Failed to start InfluxDB on %s" % (self.nd_host_name()))
        else:
            log.debug("InfluxDB is already running.")
            status = 0
        return status

    def nd_stop_influxdb(self):
        log = logging.getLogger(self.__class__.__name__ + '.' + __name__)
        log.info("Stopping InfluxDB on %s" % (self.nd_host_name()))

        status = self.nd_status_influxdb()
        if status == 0:
            status = self.nd_agent.exec_wait('service influxdb stop 2>&1 >> /dev/null', output=False)
            if status != 0:
                print "Failed to stop InfluxDB on %s" % (self.nd_host_name())
                log.warning("Failed to stop InfluxDB on %s" % (self.nd_host_name()))
        return status

    def nd_status_influxdb(self):
        log = logging.getLogger(self.__class__.__name__ + '.' + __name__)
        log.info("Checking InfluxDB status on %s" % (self.nd_host_name()))
        status = self.nd_agent.exec_wait('service influxdb status 2>&1 >> /dev/null', output=False)
        return status

    def nd_clean_influxdb(self):
        log = logging.getLogger(self.__class__.__name__ + '.' + __name__)

        status = 0
        if self.nd_run_om():
            log.info("Cleaning InfluxDB")
            ## check if influx is running on the node.  If not, start it
            influxstat = self.nd_status_influxdb()
            if influxstat != 0:
                log.info("Starting InfluxDB for clean on %s" % (self.nd_host_name()))
                status = self.nd_agent.exec_wait('service influxdb start 2>&1 >> /dev/null',
                                                 output=False,
                                                 wait_compl=False)

            for db in [ 'om-metricdb', 'om-eventdb' ]:
                status = self.dropInfluxDBDatabase(db)

            ## if influx was NOT running originally, stop it again.
            if influxstat != 0:
                status = self.nd_agent.exec_wait('service influxdb stop 2>&1 >> /dev/null', output=False)

        return status

    ##
    #
    #
    def dropInfluxDBDatabase(self, db, max_retries=8, backoff_factor=0.5):
        """Drop the InfluxDB database

        db -- the database name
        max_retries -- max number of retries (default 8)
        backoff_factor -- amount of time to factor the backoff time per retry (default 0.5)

        The backoff factor is the amount of time added for each retry.  For each retry, the
        wait time is calculated as (1 + ((retryCount-1) * backoff_factory)). The default setting
        amounts to a possible total wait time of 27 seconds.

        1 + ( 0 * 0.5 ) = 1       1.0
        1 + ( 1 * 0.5 ) = 1.5     2.5
        1 + ( 2 * 0.5 ) = 2       4.5
        1 + ( 3 * 0.5 ) = 2.5     7.0
        1 + ( 4 * 0.5 ) = 3      10.0
        1 + ( 5 * 0.5 ) = 3.5    13.5
        1 + ( 6 * 0.5 ) = 4      17.5
        1 + ( 7 * 0.5 ) = 4.5    22.0
        1 + ( 8 * 0.5 ) = 5      27.0
        1 + ( 9 * 0.5 ) = 5.5    32.5
        1 + (10 * 0.5 ) = 6      38.5
        """
        log = logging.getLogger(self.__class__.__name__ + '.' + __name__)

        status = 0
        ## setup the http requests session with an adapter configured for retries.
        s = requests.session()
        a = requests.adapters.HTTPAdapter()
        s.mount("http://", a)

        print "Removing database %s" % db
        log.debug('Removing database %s with up to %d retries @ %s seconds backoff factor' %
                  (db,max_retries,backoff_factor))

        retryCount = 0
        while retryCount < max_retries:
            retryCount += 1
            try:
                log.debug("Drop database %s [Attempt %s]" % (db, retryCount))
                response = s.delete("http://" + self.nd_host + ":8086/db/" + db + "?u=root&p=root",
                                    timeout=5)

                log.debug("Remove database %s request completed [response code %d msg: %s]" %
                          (db,response.status_code,response.text))

                if response.status_code < 200 or response.status_code >= 300:
                    if response.status_code == 400:
                        ## 400 indicates the database does not exist
                        print "InfluxDB database %s does not exist. [OK]" % db
                        log.info("InfluxDB database %s does not exist. [OK]" % (db))
                        status = 0
                        break
                    else:
                        # influx reported an unsuccessful error code.  Print the message output from the response.
                        # we don't currently distinguish between errors and things like redirects.
                        print('InfluxDB [HTTP %d] %s' % (response.status_code, response.text))

                        # todo: attempted to use log.warning, but getting error message about no handlers.
                        log.info('InfluxDB [HTTP %d] %s' % (response.status_code, response.text))
                        status = 1
                        break
                else:
                    print "Successfully dropped InfluxDB database %s" % db
                    log.info("Successfully dropped InfluxDB database %s" % (db))
                    break;

            except Exception as e:

                if retryCount < max_retries:
                    retryTime = 1 + ( (retryCount - 1) * backoff_factor )
                    print "Drop InfluxDB database %s attempt failed - Retrying in %s seconds" % (db, retryTime)
                    log.info("Drop InfluxDB database %s attempt failed - Retrying in %s seconds" % (db, retryTime))
                    time.sleep(retryTime)
                else:
                    print "Failed to drop influxDB database %s after %s retry attempts: %s" % (db, max_retries, e)
                    # todo: attempted to use log.warn, but getting error message about no handlers.
                    log.info("Failed to drop influxDB database %s after %s retry attempts: %s" % (db, max_retries, e))
                    status = 1

                continue

        ## close the http session
        s.close()

        return status

    ###
    # Start platform services in all nodes.
    #
    def nd_start_platform(self, om_ip = None, test_harness=False, _bin_dir=None, _log_dir=None):
        port_arg = '--fds-root=%s --fds.pm.id=%s' % \
                   (self.nd_conf_dict['fds_root'], self.nd_conf_dict['node-name'])
        if 'fds_port' in self.nd_conf_dict:
            port = self.nd_conf_dict['fds_port']
            port_arg = port_arg + (' --fds.pm.platform_port=%s' % port)
        else:
            port = 7000  # PM default.

        if om_ip is not None:
            port_arg = port_arg + (' --fds.common.om_ip_list=%s' % om_ip)

        fds_dir = self.nd_conf_dict['fds_root']

        if _bin_dir is None:
            bin_dir = fds_dir + '/bin'
        else:
            bin_dir = _bin_dir

        if _log_dir is None:
            log_dir = bin_dir
        else:
            log_dir = _log_dir

        print "Start platform daemon on %s in %s" % (self.nd_host_name(),fds_dir)

        # When running from the test harness, we want to wait for results
        # but not assume we are running from an FDS package install.
        if test_harness:
            status = self.nd_agent.exec_wait('bash -c \"(nohup ./platformd --fds-root=%s > %s/pm.%s.out 2>&1 &) \"' %
                                            #(bin_dir, fds_dir, log_dir if self.nd_agent.env_install else ".",
                                            (fds_dir, log_dir, port),
                                             fds_bin=True)
        else:
            status = self.nd_agent.ssh_exec_fds('platformd ' + port_arg + ' &> %s/pm.out' % log_dir)

        if status == 0:
            time.sleep(4)

        return status

    ###
    # Capture the node's assigned name and UUID.
    #
    def nd_populate_metadata(self, om_node):
        log = logging.getLogger(self.__class__.__name__ + '.' + 'nd_populate_metadata')

        if 'fds_port' in self.nd_conf_dict:
            port = self.nd_conf_dict['fds_port']
        else:
            port = 7000  # PM default.

        # From the listServices output we can determine node name
        # and node UUID.
        # Figure out the platform uuid.  Platform has 'pm' as the name
        # port should match the read port from [fdsconsole.py domain listServices] output
        # Group 0 = Entire matched expression
        # Group 1 = node UUID
        # Group 2 = Node root (not showing up)
        # Group 3 = Node ID
        # Group 4 = IPv4
        # Group 5 = IPv6
        # Group 6 = Service name
        # Group 8 = Service Type
        # Group 9 = Service UUID
        # Group 10 = Control port
        # Group 11 = Data port
        # Group 12 = Migration port
        # TODO(brian): uncomment this regex when we start capturing node root correctly
        #svc_re = re.compile(r'(0x[a-f0-9]{16})\s*([\[a-zA-z]\\]*)\s*(\d{1})\s*(\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3})'
        #                    '\s*(\d.\d.\d.\d)\s*(\w{2})\s*(\w*)\s*(0x[a-f0-9]{16})\s*'
        #                    '([A-Za-z0-9_]*)\s*(\d*)\s*(\d*)\s*(\d*)')
        # Use this regex for now
        svc_re = re.compile(r'(0x[a-f0-9]{16})(\s+)(\d{1})\s+(\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3})'
                            '\s+(\d.\d.\d.\d)\s+(\w{2})\s+(\w*)\s+(0x[a-f0-9]{16})\s+'
                            '([A-Za-z0-9_]*)\s+(\d+)\s+(\d+)\s+(\d+)')

        status, stdout = om_node.nd_agent.exec_wait('bash -c \"(./fdsconsole.py domain listServices local) \"',
                                                    return_stdin=True,
                                                    fds_tools=True)

        # Figure out the platform uuid.  Platform has 'pm' as the name
        # port should match the read port from [fdsconsole.py domain listServices] output
        if status == 0:
            for line in stdout.split('\n'):
                res = svc_re.match(line)
                if res is not None:
                    # UUID
                    assigned_uuid = res.group(1)
                    # IP
                    hostName = socket.gethostbyaddr(res.group(4))
                    ourIP = (res.group(4) == self.nd_conf_dict["ip"]) or (hostName == self.nd_conf_dict["ip"])
                    # Service name
                    assigned_name = res.group(6)
                    # Ports
                    readPort = int(res.group(11))
                    if assigned_name.lower() == 'pm' and readPort == int(port):
                        self.nd_assigned_name = assigned_name
                        self.nd_uuid = assigned_uuid
                        break

            if (self.nd_uuid is None):
                log.error("Could not get meta-data for node %s." % self.nd_conf_dict["node-name"])
                log.error("Looking for ip %s and port %s." % (self.nd_conf_dict["ip"], port))
                log.error("Results from service list:\n%s." % stdout)
                status = -1
            else:
                log.debug("Node %s has assigned name %s and UUID %s." %
                          (self.nd_conf_dict["node-name"], self.nd_assigned_name, self.nd_uuid))
        else:
            log.error("status = %s" % status)
            log.error("Results from service list:\n%s." % stdout)

        return status

    ###
    # Kill all fds daemons
    #
    def nd_cleanup_daemons(self):
        fds_dir = self.nd_conf_dict['fds_root']
        bin_dir = fds_dir + '/bin'
        sbin_dir = fds_dir + '/sbin'
        tools_dir = sbin_dir + '/tools'
        var_dir = fds_dir + '/var'
        print("\nCleanup running processes on: %s, %s" % (self.nd_host_name(), bin_dir))
        # TODO (Bao): order to kill: AM, SM/DM, OM
        # TODO WIN-936 signal handler improvements - should use a different signal
        # and give the processes a chance to shutdown cleanly
        self.nd_agent.exec_wait('pkill -9 -f com.formationds.am.Main')
        self.nd_agent.exec_wait('pkill -9 bare_am')
        self.nd_agent.exec_wait('pkill -9 AMAgent')
        self.nd_agent.exec_wait('pkill -9 StorMgr')
        self.nd_agent.exec_wait('pkill -9 DataMgr')
        self.nd_agent.exec_wait('pkill -9 orchMgr')
        self.nd_agent.exec_wait('pkill -9 platformd')
        self.nd_agent.exec_wait('pkill -9 -f com.formationds.om.Main')
        time.sleep(2)

    def nd_cleanup_daemons_with_fdsroot(self, fds_root):
        bin_dir = fds_root + '/bin'
        sbin_dir = fds_root + '/sbin'
        tools_dir = sbin_dir + '/tools'
        var_dir = fds_root + '/var'
        print("\nCleanup running processes on: %s, %s" % (self.nd_host_name(), bin_dir))
        # TODO (Bao): order to kill: AM, SM/DM, OM
        self.nd_agent.ssh_exec('pkill -9 -f \'\-\-fds\-root={}\''.format(fds_root), wait_compl=True)

    ###
    # cleanup any cores, redis, and logs.
    #
    def nd_cleanup_node(self, test_harness=False, _bin_dir=None):
        log = logging.getLogger(self.__class__.__name__ + '.' + 'nd_cleanup_node')

        fds_dir = self.nd_conf_dict['fds_root']
        bin_dir = fds_dir + '/bin'
        sbin_dir = fds_dir + '/sbin'
        tools_dir = sbin_dir + '/tools'
        dev_dir = fds_dir + '/dev'
        var_dir = fds_dir + '/var'

        ## note: needs to change if we move influx data under /fds/var/db
        influxdb_data_dir = '/opt/influxdb/shared/data/db'

        if test_harness:
            log.info("Cleanup cores/logs/redis in: %s, %s" % (self.nd_host_name(), bin_dir))

            status = self.nd_agent.exec_wait('ls ' + bin_dir)
            if status == 0:
                log.info("Cleanup cores in: %s" % bin_dir)
                self.nd_agent.exec_wait('rm -f ' + bin_dir + '/core')
                self.nd_agent.exec_wait('rm -f ' + bin_dir + '/*.core')

            status = self.nd_agent.exec_wait('ls ' + var_dir)
            if status == 0:
                log.info("Cleanup logs,db and stats in: %s" % var_dir)
                self.nd_agent.exec_wait('rm -rf ' + var_dir + '/logs')
                self.nd_agent.exec_wait('rm -rf ' + var_dir + '/db')
                self.nd_agent.exec_wait('rm -rf ' + var_dir + '/stats')

            status = self.nd_agent.exec_wait('ls /corefiles')
            if status == 0:
                log.info("Cleanup cores in: %s" % '/corefiles')
                self.nd_agent.exec_wait('rm -f /corefiles/*.core')

            status = self.nd_agent.exec_wait('ls ' + var_dir + '/core')
            if status == 0:
                log.info("Cleanup cores in: %s" % var_dir + '/core')
                self.nd_agent.exec_wait('rm -f ' + var_dir + '/core/*.core')

            status = self.nd_agent.exec_wait('ls ' + tools_dir)
            if status == 0:
                log.info("Running ./fds clean -i in %s" % tools_dir)
                os.chdir(tools_dir)
                self.nd_agent.exec_wait('%s/fds clean -i' % tools_dir)

            status = self.nd_agent.exec_wait('ls ' + dev_dir)
            if status == 0:
                log.info("Cleanup hdd-* and sdd-* in: %s" % dev_dir)
                self.nd_agent.exec_wait('rm -f ' + dev_dir + '/hdd-*')
                self.nd_agent.exec_wait('rm -f ' + dev_dir + '/ssd-*')

            status = self.nd_agent.exec_wait('ls ' + fds_dir)
            if status == 0:
                log.info("Cleanup sys-repo and user-repo in: %s" % fds_dir)
                self.nd_agent.exec_wait('rm -rf ' + fds_dir + '/sys-repo')
                self.nd_agent.exec_wait('rm -rf ' + fds_dir + '/user-repo')

            status = self.nd_agent.exec_wait('ls /dev/shm')
            if status == 0:
                log.info("Cleanup 0x* in: %s" % '/dev/shm')
                self.nd_agent.exec_wait('rm -f /dev/shm/0x*')

            log.info("Cleanup influx database in: %s" % influxdb_data_dir)
            status = self.nd_clean_influxdb()
        else:
            print("Cleanup cores/logs/redis in: %s, %s" % (self.nd_host_name(), bin_dir))
            status = self.nd_agent.exec_wait('(cd %s && rm -f core *.core); ' % bin_dir +
                '(cd %s && rm -rf logs db stats); ' % var_dir +
                '(rm -f /corefiles/*.core); '  +
                '(rm -f %s/core/*.core); ' % var_dir +
                '([ -f "%s/fds" ] && %s/fds clean -i) 2>&1>>/dev/null; ' % (tools_dir, tools_dir) +
                '(rm -rf %s/hdd-*/*); ' % dev_dir +
                '(rm -rf %s/ssd-*/*); ' % dev_dir +
                '(rm -rf %s/*-repo/); ' % fds_dir +
                '(cd /dev/shm && rm -f 0x*)')
            status = self.nd_clean_influxdb()


        if status == -1:
            # ssh_exec() returns -1 when there is output to syserr and
            # status from the command execution was 0. In this case, we'll
            # assume that the failure was due to some of these components
            # missing. But since we wanted to delete them anyway, we'll
            # call it good.
            status = 0

        return status

###
# Handle AM config section
#
class FdsAMConfig(FdsConfig):
    def __init__(self, name, items, cmd_line_options):
        super(FdsAMConfig, self).__init__(items, cmd_line_options)
        self.nd_am_node = None
        self.nd_conf_dict['am-name'] = name

    ###
    # Connect the association between AM section to the matching node that would run
    # AM there.  From the node, we can send ssh commands to start the AM.
    #
    def am_connect_node(self, nodes):
        if 'fds_node' not in self.nd_conf_dict:
            print('sh/am section must have "fds_node" keyword')
            sys.exit(1)

        name = self.nd_conf_dict['fds_node']
        for n in nodes:
            if n.nd_conf_dict['node-name'] == name:
                self.nd_am_node = n
                return

        print('Can not find matching fds_node in %s sections' % name)
        sys.exit(1)

    ###
    # Required am_connect_node()
    #
    def am_start_service(self):
        cmd = ' --fds-root=%s' % self.nd_am_node.nd_conf_dict['fds_root']

        self.nd_am_node.nd_agent.ssh_exec_fds('AMAgent' + cmd)

###
# Handle Volume config section
#
class FdsVolConfig(FdsConfig):
    def __init__(self, name, items, cmd_line_options):
        super(FdsVolConfig, self).__init__(items, cmd_line_options)
        self.nd_am_conf = None
        self.nd_am_node = None
        self.nd_conf_dict['vol-name'] = name

    def vol_connect_to_am(self, am_nodes, all_nodes):
        if 'client' not in self.nd_conf_dict:
            print('volume section must have "client" keyword')
            sys.exit(1)

        client = self.nd_conf_dict['client']
        for am in am_nodes:
            if am.nd_conf_dict['am-name'] == client:
                self.nd_am_conf = am
                self.nd_am_node = am.nd_am_node
                return

        # The system test framework does not use {sh|am] sections.
        # So look at all nodes to bind the volume to an AM.
        for n in all_nodes:
            if n.nd_conf_dict['node-name'] == client:
                self.nd_am_node = n
                return

        print('Can not find matching AM node in %s' % self.nd_conf_dict['vol-name'])
        sys.exit(1)

    def vol_create(self, cli):
        volume = self

        cmd = (' volume create %s' % volume.nd_conf_dict['vol-name'])

        if 'id' not in volume.nd_conf_dict:
            raise Exception('Volume section %s must have "id" keyword.' % volume.nd_conf_dict['vol-name'])
        cmd = cmd + (' --tenant-id %s' % volume.nd_conf_dict['id'])

        if 'access' not in volume.nd_conf_dict:
            access = 'object'
        else:
            access = volume.nd_conf_dict['access']

        # Size only makes sense for block volumes
        if 'block' == access:
            if 'size' not in volume.nd_conf_dict:
                raise Exception('Volume section %s must have "size" keyword.' % volume.nd_conf_dict['vol-name'])
            cmd = cmd + (' --blk-dev-size %s' % volume.nd_conf_dict['size'])

        cmd = cmd + (' --vol-type %s' % access)
        if 'media' not in volume.nd_conf_dict:
            media = 'hdd'
        else:
            media = volume.nd_conf_dict['media']

        cmd = cmd + (' --media-policy %s' % media)

        cli.run_cli(cmd)

    def vol_attach(self, cli):
        cmd = (' --volume-attach %s -i %s -n %s' %
               (self.nd_conf_dict['vol-name'],
                self.nd_conf_dict['id'],
                self.nd_am_conf.nd_conf_dict['am-name']))
        cli.run_cli(cmd)

    def vol_apply_cfg(self, cli):
        self.vol_create(cli)
        time.sleep(1)
        self.vol_attach(cli)

###
# Handle Volume Policy Config
#
class FdsVolPolicyConfig(FdsConfig):
    def __init__(self, name, items, cmd_line_options):
        super(FdsVolPolicyConfig, self).__init__(items, cmd_line_options)
        self.nd_conf_dict['pol-name'] = name

    def policy_apply_cfg(self, cli):
        if 'id' not in self.nd_conf_dict:
            print('Policy section must have an id')
            sys.exit(1)

        pol = self.nd_conf_dict['id']
        cmd = (' qospolicy create policy_%s ' % (pol))

        if 'iops_min' in self.nd_conf_dict:
            cmd = cmd + (' %s' % self.nd_conf_dict['iops_min'])

        if 'iops_max' in self.nd_conf_dict:
            cmd = cmd + (' %s' % self.nd_conf_dict['iops_min'])

        if 'priority' in self.nd_conf_dict:
            cmd = cmd + (' %s' % self.nd_conf_dict['priority'])

        cli.run_cli(cmd)

###
# Handler user setup
#
class FdsUserConfig(FdsConfig):
    def __init__(self, name, items, cmd_line_options):
        super(FdsUserConfig, self).__init__(items, cmd_line_options)

###
# Handle cli related
#
class FdsCliConfig(FdsConfig):
    def __init__(self, name, items, cmd_line_options):
        super(FdsCliConfig, self).__init__(items, cmd_line_options)
        self.nd_om_node = None
        self.nd_conf_dict['cli-name'] = name

    ###
    # Find the OM node that we'll run this CLI from that node.
    #
    def bind_to_om(self, nodes):
        for n in nodes:
            if n.nd_run_om() == True:
                self.nd_om_node = n
                return n
        self.nd_om_node = nodes[0]
        return nodes[0]

    ###
    # Pass arguments to fdsconsole running on OM node.
    #
    def run_cli(self, command):
        if self.nd_om_node is None:
            print "You must bind to an OM node first"
            sys.exit(1)

        om = self.nd_om_node
        self.debug_print("Run %s on OM %s" % (command, om.nd_host_name()))

        om.nd_agent.exec_wait(
            ('fdsconsole.py ') + command,
            wait_compl=True,
            fds_tools=True)

###
# Run a specific test step
#
class FdsScenarioConfig(FdsConfig):
    def __init__(self, name, items, cmd_line_options):
        super(FdsScenarioConfig, self).__init__(items, cmd_line_options)
        self.nd_conf_dict['scenario-name'] = name
        self.cfg_sect_user    = []
        self.cfg_sect_nodes   = []
        self.cfg_sect_ams     = []
        self.cfg_sect_vol_pol = []
        self.cfg_sect_volumes = []
        self.cfg_sect_cli     = []

    def start_scenario(self):
        assert(len(self.cfg_sect_cli) == 1)
        cli    = self.cfg_sect_cli[0]
        script = self.nd_conf_dict['script']
        if 'delay_wait' in self.nd_conf_dict:
            delay = int(self.nd_conf_dict['delay_wait'])
        else:
            delay = 1
        if 'wait_completion' in self.nd_conf_dict:
            wait  = self.nd_conf_dict['wait_completion']
        else:
            wait  = 'false'
        if re.match('\[node.+\]', script) != None:
            for s in self.cfg_sect_nodes:
                if '[' + s.nd_conf_dict['node-name'] + ']' == script:
                    s.nd_start_om()
                    s.nd_start_platform(self.cfg_om)
                    break
        elif re.match('\[sh.+\]', script) != None:
            for s in self.cfg_sect_ams:
                if '[' + s.nd_conf_dict['am-name'] + ']' == script:
                    s.am_start_service()
                    break
        elif re.match('\[policy.+\]', script) != None:
            for s in self.cfg_sect_vol_pol:
                if '[' + s.nd_conf_dict['pol-name'] + ']' == script:
                    s.policy_apply_cfg(cli)
                    break
        elif re.match('\[volume.+\]', script) != None:
            for s in self.cfg_sect_volumes:
                if '[' + s.nd_conf_dict['vol-name'] + ']' == script:
                    s.vol_apply_cfg(cli)
                    break
        elif re.match('\[fdscli.*\]', script) != None:
            for s in self.cfg_sect_cli:
                if '[' + s.nd_conf_dict['cli-name'] + ']' == script:
                    s.run_cli(self.nd_conf_dict['script_args'])
                    break
        else:
            if 'script_args' in self.nd_conf_dict:
                script_args = self.nd_conf_dict['script_args']
            else:
                script_args = ''
            print "Running external script: ", script, script_args
            args = script_args.split()
            p1 = subprocess.Popen([script] + args)
            if wait == 'true':
                p1.wait()

        if delay > 1:
            print 'Delaying for %d seconds' % delay
        time.sleep(delay)

    def sce_bind_sections(self, user, nodes, am, vol_pol, volumes, cli, om):
        self.cfg_sect_user    = user
        self.cfg_sect_nodes   = nodes
        self.cfg_sect_ams     = am
        self.cfg_sect_vol_pol = vol_pol
        self.cfg_sect_volumes = volumes
        self.cfg_sect_cli     = cli
        self.cfg_om           = om

class FdsIOBlockConfig(FdsConfig):
    def __init__(self, name, items, cmd_line_options):
        super(FdsIOBlockConfig, self).__init__(items, cmd_line_options)
        self.nd_conf_dict['io-block-name'] = name

class FdsDatagenConfig(FdsConfig):
    def __init__(self, name, items, cmd_line_options):
        super(FdsDatagenConfig, self).__init__(items, cmd_line_options)
        self.nd_conf_dict['datagen-name'] = name

    def dg_parse_blocks(self):
        blocks = self.nd_conf_dict['rand_blocks']
        self.nd_conf_dict['rand_blocks'] = re.split(',', blocks)

        blocks = self.nd_conf_dict['dup_blocks']
        self.nd_conf_dict['dup_blocks'] = re.split(',', blocks)

###
# Handle install section
#
class FdsPkgInstallConfig(FdsConfig):
    def __init__(self, name, items, cmd_line_options):
        super(FdsPkgInstallConfig, self).__init__(items, cmd_line_options)

###
# Handle fds bring up config parsing
#
class FdsConfigFile(object):
    def __init__(self, cfg_file, verbose = False, dryrun = False, install = False, inventory_file =None, rt_env=None):
        self.cfg_file      = cfg_file
        self.cfg_verbose   = verbose
        self.cfg_dryrun    = dryrun
        self.cfg_install   = []
        self.cfg_am        = []
        self.cfg_user      = []
        self.cfg_nodes     = []
        self.cfg_volumes   = []
        self.cfg_vol_pol   = []
        self.cfg_scenarios = []
        self.cfg_io_blocks = []
        self.cfg_datagen   = []
        self.cfg_cli       = None
        self.cfg_om        = None
        self.cfg_parser    = None
        self.cfg_localHost = None
        self.inventory = inventory_file
        self.cfg_is_fds_installed = install
        self.rt_env = rt_env

    def config_parse(self):
        cmd_line_options = {
            'verbose': self.cfg_verbose,
            'dryrun' : self.cfg_dryrun,
            'install': self.cfg_is_fds_installed,
            'inventory_file' : self.inventory
        }
        self.cfg_parser = ConfigParser.ConfigParser()
        self.cfg_parser.read(self.cfg_file)
        if len (self.cfg_parser.sections()) < 1:
            print 'ERROR:  Unable to open or parse the config file "%s".' % (self.cfg_file)
            sys.exit (1)

        nodeID = 0
        for section in self.cfg_parser.sections():
            items = self.cfg_parser.items(section)
            if re.match('user', section) != None:
                self.cfg_user.append(FdsUserConfig('user', items, cmd_line_options))

            elif re.match('node', section) != None:
                n = None
                items_d = dict(items)
                if 'enable' in items_d:
                    if items_d['enable'] == 'true':
                        n = FdsNodeConfig(section, items, cmd_line_options, nodeID, self.rt_env)
                else:
                    # Store OM ip address to pass to PMs during scenarios
                    if 'om' in items_d:
                        if items_d['om'] == 'true':
                            self.cfg_om = items_d['ip']

                    n = FdsNodeConfig(section, items, cmd_line_options, nodeID, self.rt_env)

                if n is not None:
                    n.nd_nodeID = nodeID

                    if "services" in n.nd_conf_dict:
                        n.nd_services = n.nd_conf_dict["services"]
                    else:
                        n.nd_services = "dm,sm,am"

                    self.cfg_nodes.append(n)
                    nodeID = nodeID + 1

            elif (re.match('sh', section) != None) or (re.match('am', section) != None):
                self.cfg_am.append(FdsAMConfig(section, items, cmd_line_options))

            elif re.match('policy', section) != None:
                self.cfg_vol_pol.append(FdsVolPolicyConfig(section, items, cmd_line_options))

            elif re.match('volume', section) != None:
                self.cfg_volumes.append(FdsVolConfig(section, items, cmd_line_options))
            elif re.match('scenario', section)!= None:
                self.cfg_scenarios.append(FdsScenarioConfig(section, items, cmd_line_options))
            elif re.match('io_block', section) != None:
                self.cfg_io_blocks.append(FdsIOBlockConfig(section, items, cmd_line_options))
            elif re.match('datagen', section) != None:
                self.cfg_datagen.append(FdsDatagenConfig(section, items, cmd_line_options))
            elif re.match('install', section) != None:
                self.cfg_install.append(FdsPkgInstallConfig(section, items, cmd_line_options))
            else:
                print "Unknown section", section

        if self.cfg_cli is None:
            self.cfg_cli = FdsCliConfig("fdscli", None, cmd_line_options)
            self.cfg_cli.bind_to_om(self.cfg_nodes)

        for am in self.cfg_am:
            am.am_connect_node(self.cfg_nodes)

        for vol in self.cfg_volumes:
            vol.vol_connect_to_am(self.cfg_am, self.cfg_nodes)

        for sce in self.cfg_scenarios:
            sce.sce_bind_sections(self.cfg_user, self.cfg_nodes,
                                  self.cfg_am, self.cfg_vol_pol,
                                  self.cfg_volumes, [self.cfg_cli],
                                  self.cfg_om)

        for dg in self.cfg_datagen:
            dg.dg_parse_blocks()

        # Generate a localhost node instance so that we have a
        # node agent to execute commands locally when necessary.
        items = [('enable', True), ('ip','localhost'), ('fds_root', '/fds')]
        self.cfg_localhost = FdsNodeConfig("localhost", items, cmd_line_options, rt_env=self.rt_env)


###
# Parse the config file and setup runtime env for it.
#
class FdsConfigRun(object):
    def __init__(self, env, opt, test_harness=False):
        self.rt_om_node = None
        self.rt_my_node = None
        if opt.config_file is None:
            print "You need to pass config file [-f config]"
            sys.exit(1)

        self.rt_env = env
        if self.rt_env is None:
            self.rt_env = inst.FdsEnv(opt.fds_root, _install=opt.install, _fds_source_dir=opt.fds_source_dir,
                                      _verbose=opt.verbose, _test_harness=test_harness)

        self.rt_obj = FdsConfigFile(opt.config_file, opt.verbose, opt.dryrun, opt.install, opt.inventory_file, self.rt_env )
        self.rt_obj.config_parse()

        # Fixup user/passwd in runtime env from config file.
        users = self.rt_obj.cfg_user
        if users is not None:
            # Should only be one of these guys.
            usr = users[0]
            self.rt_env.env_user     = usr.get_config_val('user_name')
            self.rt_env.env_password = usr.get_config_val('password')

            if self.rt_env.env_test_harness:
                # Get sudo password from the commandline if there. If not,
                # check the config file.
                if opt.sudo_password is not None:
                    self.rt_env.env_sudo_password = opt.sudo_password
                elif 'sudo_password' in usr.nd_conf_dict:
                    self.rt_env.env_sudo_password = usr.get_config_val('sudo_password')

        # Setup ssh agent to connect all nodes and find the OM node.

        if hasattr (opt, 'target') and opt.target:
            print "Target specified.  Applying commands against target: {}".format (opt.target)
            self.rt_obj.cfg_nodes  = [node for node in self.rt_obj.cfg_nodes
                if node.nd_conf_dict['node-name'] == opt.target]
            if len (self.rt_obj.cfg_nodes) is 0:
                print "No matching nodes for the target '%s'" %(opt.target)
                sys.exit(1)

        quiet_ssh = False

        if hasattr (opt, 'ssh_quiet') and opt.ssh_quiet:
            quiet_ssh = True

        nodes = self.rt_obj.cfg_nodes

        for n in nodes:
            n.nd_connect_agent(self.rt_env, quiet_ssh)
            n.nd_agent.setup_env('')

            if n.nd_run_om() == True:
                self.rt_om_node = n

        # Set up our localhost "node" as well.
        self.rt_obj.cfg_localhost.nd_connect_agent(self.rt_env, quiet_ssh)
        self.rt_obj.cfg_localhost.nd_agent.setup_env('')

    def rt_get_obj(self, obj_name):
        return getattr(self.rt_obj, obj_name)

    def rt_fds_bootstrap(self, run_om = True):
        if self.rt_om_node == None:
            print "You must have OM node in your config"
            sys.exit(1)

        om_node = self.rt_om_node
        if 'ip' not in om_node.nd_conf_dict:
            print "You need to have IP in the OM node of your config"
            sys.exit(1)

        om_ip = om_node.nd_conf_dict['ip']
        for n in self.rt_obj.cfg_nodes:
            n.nd_start_platform(om_ip)

        if run_om == True:
            om_node.nd_start_om()

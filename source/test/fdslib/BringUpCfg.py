#!/usr/bin/env python

# Copyright 2014 by Formations Data Systems, Inc.
#
import ConfigParser
import FdsSetup as inst
import re, sys
import pdb, time

###
# Base config class, which is key/value dictionary.
#
class FdsConfig(object):
    def __init__(self, items, verbose):
        self.nd_verbose   = verbose
        self.nd_conf_dict = {}
        if items is not None:
            for it in items:
                self.nd_conf_dict[it[0]] = it[1]

    def get_config_val(self, key):
        return self.nd_conf_dict[key]

    def debug_print(self):
        print self.nd_conf_dict

###
# Handle node config section
#
class FdsNodeConfig(FdsConfig):
    def __init__(self, name, items, verbose):
        super(FdsNodeConfig, self).__init__(items, verbose)
        self.nd_conf_dict['node-name'] = name
        self.nd_rmt_host  = None
        self.nd_rmt_agent = None
        self.nd_package   = None
        self.nd_local_env = None

    ###
    # Establish ssh connection with the remote node.  After this call, the obj
    # can use nd_rmt_host to send ssh commands to the remote node.
    #
    def nd_connect_rmt_agent(self, env):
        if 'fds_root' in self.nd_conf_dict:
            root = self.nd_conf_dict['fds_root']
            self.nd_rmt_agent = inst.FdsRmtEnv(root, self.nd_verbose)

            self.nd_rmt_agent.env_user     = env.env_user
            self.nd_rmt_agent.env_password = env.env_password
        else:
            print "Missing fds-root keyword in the node section"
            sys.exit(1)
        
        if 'ip' not in self.nd_conf_dict:
            print "Missing ip keyword in the node section"
            sys.exit(1)

        self.nd_local_env = env
        self.nd_rmt_host  = self.nd_conf_dict['ip']

        print "Making ssh connection to", self.nd_host_name()
        self.nd_rmt_agent.ssh_connect(self.nd_rmt_host)

    ###
    # Install the tar ball package at local location to the remote node.
    #
    def nd_install_rmt_pkg(self):
        if self.nd_rmt_agent is None:
            print "You need to call nd_connect_rmt_agent() first"
            sys.exit(0)

        print "Installing FDS package to:", self.nd_host_name()
        pkg = inst.FdsPackage(self.nd_rmt_agent)
        pkg.package_install(self.nd_rmt_agent, self.nd_local_env.get_pkg_tar())

    def nd_host_name(self):
        return '[' + self.nd_rmt_host + '] ' + self.nd_conf_dict['node-name']

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
    def nd_start_om(self):
        if self.nd_run_om():
            if self.nd_rmt_agent is None:
                print "You need to call nd_connect_rmt_agent() first"
                sys.exit(0)

            fds_dir = self.nd_conf_dict['fds_root']
            bin_dir = fds_dir + '/bin'
            print "\nStart OM in", self.nd_host_name()
            self.nd_rmt_agent.ssh_exec_fds(
                "orchMgr --fds-root=%s > %s/om.out" %
                    (self.nd_conf_dict['fds_root'], bin_dir))
            time.sleep(2)
            return 1
        return 0

    ###
    # Start platform services in all nodes.
    #
    def nd_start_platform(self):
        port_arg = '--fds-root=%s' % self.nd_conf_dict['fds_root']
        if 'fds_port' in self.nd_conf_dict:
            port = self.nd_conf_dict['fds_port']
            port_arg = port_arg + (' --fds.plat.control_port=%s' % port)

        fds_dir = self.nd_conf_dict['fds_root']
        bin_dir = fds_dir + '/bin'
        print "\nStart platform daemon in", self.nd_host_name()
        self.nd_rmt_agent.ssh_exec_fds('platformd ' + port_arg +
            ' > %s/pm.out' % bin_dir)
        time.sleep(4)

    ###
    # Kill all fds daemons
    #
    def nd_cleanup_daemons(self):
        fds_dir = self.nd_conf_dict['fds_root']
        bin_dir = fds_dir + '/bin'
        sbin_dir = fds_dir + '/sbin'
        tools_dir = sbin_dir + '/tools'
        var_dir = fds_dir + '/var'
        print("\nCleanup running processes in: %s, %s" % (self.nd_host_name(), bin_dir))
        # TODO (Bao): order to kill: AM, SM/DM, OM
        self.nd_rmt_agent.ssh_exec('pkill -9 Mgr; pkill -9 AMAgent; pkill -9 platformd; '
           'pkill -9 -f com.formationds.web.om.Main;', wait_compl=True)

    ###
    # cleanup any cores, redis, and logs.
    #
    def nd_cleanup_node(self):
        fds_dir = self.nd_conf_dict['fds_root']
        bin_dir = fds_dir + '/bin'
        sbin_dir = fds_dir + '/sbin'
        tools_dir = sbin_dir + '/tools'
        var_dir = fds_dir + '/var'
        print("\nCleanup cores/logs/redis in: %s, %s" % (self.nd_host_name(), bin_dir))
        self.nd_rmt_agent.ssh_exec('(cd %s && rm core *.core); ' % bin_dir +
            '(cd %s && rm -r logs stats); ' % var_dir +
            '(cd /corefiles && rm *.core); '  +
            '(cd %s && ./fds clean -i)' % tools_dir, wait_compl=True)

###
# Handle AM config section
#
class FdsAMConfig(FdsConfig):
    def __init__(self, name, items, verbose):
        super(FdsAMConfig, self).__init__(items, verbose)
        self.nd_am_node = None
        self.nd_conf_dict['am-name'] = name

    ###
    # Connect the association between AM section to the matching node that would run
    # AM there.  From the node, we can send ssh commands to start the AM.
    #
    def am_connect_node(self, nodes):
        if 'fds_node' not in self.nd_conf_dict:
            print('sh section must have "fds-node" keyword')
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
        cmd = (' --fds-root=%s --node-name=%s' %
               (self.nd_am_node.nd_conf_dict['fds_root'],
                self.nd_conf_dict['am-name']))

        self.nd_am_node.nd_rmt_agent.ssh_exec_fds('AMAgent' + cmd)

###
# Handle Volume config section
#
class FdsVolConfig(FdsConfig):
    def __init__(self, name, items, verbose):
        super(FdsVolConfig, self).__init__(items, verbose)
        self.nd_am_conf = None
        self.nd_am_node = None
        self.nd_conf_dict['vol-name'] = name

    def vol_connect_to_am(self, am_nodes):
        if 'client' not in self.nd_conf_dict:
            print('volume section must have "client" keyword')
            sys.exit(1)

        client = self.nd_conf_dict['client']
        for am in am_nodes:
            if am.nd_conf_dict['am-name'] == client:
                self.nd_am_conf = am
                self.nd_am_node = am.nd_am_node
                return

        print('Can not find matcing AM node in %s' % self.nd_conf_dict['vol-name'])
        sys.exit(1)

    def vol_create(self, cli):
        cmd = (' --volume-create %s' % self.nd_conf_dict['vol-name'])
        if 'id' not in self.nd_conf_dict:
            print('Volume section must have "id" keyword')
            sys.exit(1)

        cmd = cmd + (' -i %s' % self.nd_conf_dict['id'])
        if 'size' not in self.nd_conf_dict:
            print('Volume section must have "size" keyword')
            sys.exit(1)

        cmd = cmd + (' -s %s' % self.nd_conf_dict['size'])
        if 'policy' not in self.nd_conf_dict:
            print('Volume section must have "policy" keyword')
            sys.exit(1)

        cmd = cmd + (' -p %s' % self.nd_conf_dict['policy'])
        if 'access' not in self.nd_conf_dict:
            access = 's3'
        else:
            access = self.nd_conf_dict['access']

        cmd = cmd + (' -y %s' % access)
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
    def __init__(self, name, items, verbose):
        super(FdsVolPolicyConfig, self).__init__(items, verbose)
        self.nd_conf_dict['pol-name'] = name

    def policy_apply_cfg(self, cli):
        if 'id' not in self.nd_conf_dict:
            print('Policy section must have an id')
            sys.exit(1)

        pol = self.nd_conf_dict['id']
        cmd = (' --policy-create policy_%s -p %s' % (pol, pol))

        if 'iops_min' in self.nd_conf_dict:
            cmd = cmd + (' -g %s' % self.nd_conf_dict['iops_min'])

        if 'iops_max' in self.nd_conf_dict:
            cmd = cmd + (' -m %s' % self.nd_conf_dict['iops_max'])

        if 'priority' in self.nd_conf_dict:
            cmd = cmd + (' -r %s' % self.nd_conf_dict['priority'])

        cli.run_cli(cmd)

###
# Handler user setup
#
class FdsUserConfig(FdsConfig):
    def __init__(self, name, items, verbose):
        super(FdsUserConfig, self).__init__(items, verbose)

###
# Handle cli related
#
class FdsCliConfig(FdsConfig):
    def __init__(self, name, items, verbose):
        super(FdsCliConfig, self).__init__(items, verbose)
        self.nd_om_node = None

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
    # Pass arguments to fdscli running on OM node.
    #
    def run_cli(self, command):
        if self.nd_om_node is None:
            print "You must bind to an OM node first"
            sys.exit(1)

        om = self.nd_om_node
        if (self.nd_verbose):
            print "Run %s on OM %s" % (command, om.nd_host_name())

        om.nd_rmt_agent.ssh_exec_fds(
            ('fdscli --fds-root %s ') % om.nd_rmt_agent.get_fds_root() + command,
            wait_compl=True)

###
# Handle fds bring up config parsing
#
class FdsConfig(object):
    def __init__(self, cfg_file, verbose = False):
        self.cfg_file    = cfg_file
        self.cfg_verbose = verbose
        self.cfg_am      = []
        self.cfg_user    = []
        self.cfg_nodes   = []
        self.cfg_volumes = []
        self.cfg_vol_pol = []
        self.cfg_cli     = None
        self.cfg_parser  = ConfigParser.ConfigParser()

    def config_parse(self):
        verbose = self.cfg_verbose
        self.cfg_parser.read(self.cfg_file)
        for section in self.cfg_parser.sections():
            items = self.cfg_parser.items(section)
            if re.match('user', section) != None:
                self.cfg_user.append(FdsUserConfig('user', items, verbose))

            elif re.match('node', section) != None:
                if 'enable' in items:
                    if items['enable'] == 'true':
                        self.cfg_nodes.append(FdsNodeConfig(section, items, verbose))
                else:
                    self.cfg_nodes.append(FdsNodeConfig(section, items, verbose))

            elif re.match('sh', section) != None:
                self.cfg_am.append(FdsAMConfig(section, items, verbose))

            elif re.match('policy', section) != None:
                self.cfg_vol_pol.append(FdsVolPolicyConfig(section, items, verbose))

            elif re.match('volume', section) != None:
                self.cfg_volumes.append(FdsVolConfig(section, items, verbose))
            else:
                print "Unknown section", section

        if self.cfg_cli is None:
            self.cfg_cli = FdsCliConfig("fds-cli", None, verbose)
            self.cfg_cli.bind_to_om(self.cfg_nodes)

        for am in self.cfg_am:
            am.am_connect_node(self.cfg_nodes)

        for vol in self.cfg_volumes:
            vol.vol_connect_to_am(self.cfg_am)

    # TODO(Vy): actually we don't need these C++ style accessor functions.
    # In Python, we can always use decorator to intercept get/set and all member
    # data are public anyway.
    #
    def config_nodes(self):
        return self.cfg_nodes

    def config_user(self):
        return self.cfg_user

    def config_am(self):
        return self.cfg_am

    def config_volumes(self):
        return self.cfg_volumes

    def config_vol_policy(self):
        return self.cfg_vol_pol

    def config_cli(self):
        return self.cfg_cli

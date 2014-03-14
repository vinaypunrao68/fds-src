#!/usr/bin/env python

# Copyright 2014 by Formations Data Systems, Inc.
#
import ConfigParser
import FdsSetup as inst
import re, sys
import pdb, time

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
        self.nd_rmt_agent.ssh_connect(self.nd_rmt_host)

    def nd_install_rmt_pkg(self):
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
    def nd_start_om(self, count):
        if self.nd_run_om() and count == 0:
            print "Start OM in", self.nd_host_name()
            return 1
        return 0

    ###
    # Start platform services in all nodes.
    #
    def nd_start_platform(self):
        port_arg = ''
        if 'fds_port' in self.nd_conf_dict:
            port = self.nd_conf_dict['fds_port']
            port_arg = ' --fds.plat.control_port=%s' % port

###
# Handle AM config section
#
class FdsAMConfig(FdsConfig):
    def __init__(self, name, items, verbose):
        super(FdsAMConfig, self).__init__(items, verbose)
        self.nd_conf_dict['am-name'] = name

###
# Handle Volume config section
#
class FdsVolConfig(FdsConfig):
    def __init__(self, name, items, verbose):
        super(FdsVolConfig, self).__init__(items, verbose)
        self.nd_conf_dict['vol-name'] = name

###
# Handle Volume Policy Config
#
class FdsVolPolicyConfig(FdsConfig):
    def __init__(self, name, items, verbose):
        super(FdsVolPolicyConfig, self).__init__(items, verbose)
        self.nd_conf_dict['pol-name'] = name

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

    def bind_to_om(self, nodes):
        for n in nodes:
            if n.nd_run_om() == True:
                self.nd_om_node = n
                return n

        print "Missing section to specify OM node"
        sys.exit(1)

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

        om.nd_rmt_agent.ssh_exec_fds("fdscli" + command)

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

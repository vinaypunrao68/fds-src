#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import FdsSetup as inst
import BringUpCfg as fdscfg
import FdsNode


class NodeNotFoundException(Exception):
    pass


class FdsCluster:

    def __init__(self, config_file):
        """
        Takes FdsConfig object
        """
        self.config = fdscfg.FdsConfigFile(config_file, verbose=True)
        self.config.config_parse()
        self.env = inst.FdsEnv('.')

        self.nodes = {}
        
        # Find the node with om set and run om
        self.__om_node_id = None
        self.__run_om()

    def start_ams(self):
        """
        Stars AMs.
        TODO(Rao): This isn't needed.  Running AM should be specified in the config
        """
        for am in self.config.config_am():
            am.am_start_service()
    
    def add_node(self, node_id, activate_services = None):
        """
        Adds node.
        node_id: Id from the config
        """
        # Find node config
        node_cfg = self.__get_node_config(node_id)

        # Add the node
        if self.nodes.has_key(node_id):
            raise Exception('Node {} already exits'.format(node_id))
        self.nodes[node_id] = FdsNode(node_cfg)

        # On OM node we don't need initiate ssh connection.
        # It's already done in the constructor
        if node_cfg.nd_conf_dict['node-name'] != self.__om_node_id:
            node_cfg.nd_connect_rmt_agent(self.env)
            node_cfg.nd_rmt_agent.ssh_setup_env('')
            
        # Run platform daemon
        node_cfg.nd_start_platform()

        # activate services
        for service_id in activate_services:
            # NOTE: Passing node_cfg as service config
            self.add_service(node_id, service_id, node_cfg)

        # TODO(Rao): If AM is specified activate at the end
            
    def remove_node(self, node_id):
        cli = self.config.config_cli()
        cli.run_cli('--activate-nodes abc -k 1 -e {}'.format(service_id))


    def get_service(self, node_id, service_id):
        """
        Returns service identified by service_id
        """
        return self.nodes[node_id].services[service_id]

    def add_service(self, node_id, service_id, service_cfg):
        """
        Adds service identified by service_id
        """
        if self.nodes.has_key(node_id) is False:
            raise Exception("Node {} isn't added yet".format(node_id))

        # TODO(Rao): This part should should move into FdsNode class
        if service_id == 'sm':
            self.nodes[node_id].add_service(service_id, SMService(service_cfg))
        elif service_id == 'dm':
            self.nodes[node_id].add_service(service_id, DMService(service_cfg))
        elif service_id == 'am':
            self.nodes[node_id].add_service(service_id, AMService(service_cfg))
        elif service_id == 'om':
            self.nodes[node_id].add_service(service_id, OMService(service_cfg))
        else:
            raise Exception("Uknown service: {}".format(service_id))

        # Activate service
        cli = self.config.config_cli()
        cli.run_cli('--activate-nodes abc -k 1 -e {}'.format(service_id))

    def run_checker(self):
        """
        Runs checker process against the cluster
        TODO(Rao): Implement this
        """
        pass

    def run_dirbased_checker(self, dir = None):
        """
        Runs directory based checker on the cluster
        """
        cmd = 'checker'
        if dir:
            cmd = 'checker --fds.checker.path={}'.format(dir)
        self.__get_node_config(self.__om_node_id).nd_rmt_agent.ssh_exec_fds(
            cmd, wait_compl=True)
    
    def __get_node_config(self, node_id):
        """
        returns node config identified by node_id
        """
        for n in self.config.config_nodes():
            if n.nd_conf_dict['node-name'] == node_id:
                return n
        raise NodeNotFoundException(str(node_id))

    def __run_om(self):
        """
        Parses the node cofig and finds the node with OM and runs OM
        """
        for n in self.config.config_nodes():
            if n.nd_run_om():
                n.nd_connect_rmt_agent(self.env)
                n.nd_rmt_agent.ssh_setup_env('')
                n.nd_start_om()
                # cache om node id
                self.__om_node_id = n.nd_conf_dict['node-name']
                return 
        raise NodeNotFoundException("OM node not found")


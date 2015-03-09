###############################################################################
# author: Philippe Ribeiro
# email: philippe@formationds.com
# copyright: Formation Data Systems (2015) 
# import the Python defined libraries
###############################################################################
import logging
import os
import paramiko
import random
import subprocess
import sys

###############################################################################
# libraries defined within the scale framework
###############################################################################
import config
import config_parser
import ssh

class FdsEngine(object):
    
    '''
    This base class defines the common methods shared by the other FDS
    processes, which inherit from FdsEngine. 
    A few common features are start / stop all the nodes in the cluster.

    Parameters:
    -----------
    The inventory file, where the OM / cluster ip addresses can be found.
    '''
    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)
    
    def __init__(self, inventory_file):
        self.ip_addresses = config_parser.get_ips_from_inventory(inventory_file)
        self.om_ip_address = config_parser.get_om_ipaddress_from_inventory(inventory_file)

    def start_single_node_process(self, ip):
        pass
    
    def stop_single_node_process(self, ip):
        pass
    
    def start_all_nodes_processes(self):
        '''
        Runs fds-tool.py to start the cluster, after they have been
        shutdown. Uses paramiko's SSH to execute a command in the remote server.
        '''
        local_ssh = ssh.SSHConn(self.om_ip_address, config.SSH_USER,
                                config.SSH_PASSWORD)
        cluster = config.START_ALL_CLUSTER % config.FORMATION_CONFIG
        cmd = "cd %s && %s" % (config.FDS_SBIN, cluster)
        local_ssh.channel.exec_command(cmd)
        self.log.info(local_ssh.channel.recv(1024))
    
    def stop_all_nodes_processes(self):
        '''
        Runs fds-tool.py to stop the cluster, if it is still running.
        Uses paramiko's SSH to execute a command in the remote server.
        '''
        local_ssh = ssh.SSHConn(self.om_ip_address, config.SSH_USER,
                                config.SSH_PASSWORD)
        cluster = config.STOP_ALL_CLUSTER % config.FORMATION_CONFIG
        cmd = "cd %s && %s" % (config.FDS_SBIN, cluster)
        local_ssh.channel.exec_command(cmd)
        self.log.info(local_ssh.channel.recv(1024))
    
class AMEngine(FdsEngine):
    
    def __init__(self):
        pass
    
class OMEngine(FdsEngine):
    
    def __init__(self):
        pass
    
class PMEngine(FdsEngine):
    
    def __init__(self):
        pass
    
class SMEngine(FdsEngine):
    
    def __init__(self):
        pass
    
class DMEngine(FdsEngine):
    
    def __init__(self):
        pass
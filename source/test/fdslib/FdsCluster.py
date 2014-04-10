#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import subprocess
import FdsSetup as inst
import BringUpCfg as fdscfg
import optparse, sys, time

"""
Runs the program at prog_path with args as arguments
TODO(Rao): Don't ignore wait_time_sec
"""
def run(prog_path, args = '', wait_time_sec = 0):
    arglist = []
    arglist.append(prog_path)
    arglist.extend(args.split())
    output = subprocess.check_output(arglist)
    print output

def run_async(prog_path, args):
    pass


class NodeNotFoundException(Exception):
    pass

class FdsCluster:
    """
    Takes FdsConfig object
    """
    def __init__(self, config_file):
        self.config = fdscfg.FdsConfigFile(options.config_file, verbose=True)
        self.config.config_parse()
        self.env = inst.FdsEnv('.')
        
        # Find the node with om set and run om
        self.__run_om()
        
#         user = cfg.config_user()
#         if user is not None:
#             usr = user[0]
#             env.env_user     = usr.get_config_val('user_name')
#             env.env_password = usr.get_config_val('password')                
    """
    Stars AMs.
    TODO(Rao): This isn't needed.  Running AM should be specified in the config
    """
    def start_AMs(self):
        for am in self.config.config_am():
            am.am_start_service()
    
    """
    Adds node.
    node_id: Id from the config
    """    
    def add_node(self, node_id, services = ['sm', 'dm']):
        # Find node config
        n = self.__get_node_config(node_id)
        
        # On OM node we don't need initiate ssh connection.
        # It's already done in the constructor
        if n.nd_conf_dict['node-name'] != self.__om_node_id:
            n.nd_connect_rmt_agent(self.env)
            n.nd_rmt_agent.ssh_setup_env('')
            
        # Run platform daemon
        n.nd_start_platform()

        # start services
        cli = self.config.config_cli()
        # Activate services
        cli.run_cli('--activate-nodes abc -k 1 -e {}'.format(','.join(services)))
        # TODO(Rao): If am is to be run, run AM
            
    def remove_node(self, node_id):
        pass
    
    """
    Runs checker process agains the cluster
    """
    def run_checker(self):
        pass
    
    """
    returns node config identified by node_id
    """
    def __get_node_config(self, node_id):
        for n in self.config.config_nodes():
            if n.nd_conf_dict['node-name'] == node_id:
                return n
        raise NodeNotFoundException(str(node_id))

    """
    Parses the node cofig and finds the node with OM and runs OM
    """
    def __run_om(self):
        for n in self.config.config_nodes():
            if n.nd_run_om():
                n.nd_connect_rmt_agent(self.env)
                n.nd_rmt_agent.ssh_setup_env('')
                n.nd_start_om()
                # cache om node id
                self.__om_node_id = n.nd_conf_dict['node-name']
                return 
        raise NodeNotFoundException("OM node not found")

if __name__ == '__main__':
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option('-f', '--file', dest = 'config_file',
                      help = 'configuration file (e.g. sample.cfg)', metavar = 'FILE')
    parser.add_option('-v', '--verbose', action = 'store_true', dest = 'verbose',
                      help = 'enable verbosity')

    (options, args) = parser.parse_args()


    if options.config_file is None:
        print "You need to pass config file"
        sys.exit(1)
    
    cluster = FdsCluster(options.config_file)
    time.sleep(5)
    cluster.start_AMs()
    time.sleep(5)
    cluster.add_node('node1')

    run('/home/nbayyana/bin/uploads3.sh')
    
    time.sleep(2)
    
    #cluster.add_node('node2')

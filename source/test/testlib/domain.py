##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: ec2.py                                                               #
##############################################################################
import Queue as queue
import random
import threading
import time

import ec2
import utils.helper as helper

class Domain(object):
    
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    
    def __init__(self, name, cluster_size=4):

        self.cluster_size = 0
        self.name = name
        self.has_started = False
        if cluster_size < 1:
            self.cluster_size=4
        else:
            self.cluster_size = cluster_size
        # in case of a multi OM cluster
        self.nodes = []
        self.om_ip_address=[]
        
    
    def check_status(self):
        # will make use of the fabric APIs to check each node
        running = []
        halted = []
        terminated = []
        for node in self.nodes:
            if node.get_instance_status() == 'running':
                running.append(node)
            elif node.get_instance_status() == 'stopped':
                halted.append(node)
            else:
                terminated.append(node)
        
        self.log.info("%s/%s nodes running" % (len(running), self.cluster_size))
        self.log.info("%s/%s nodes halted" % (len(halted), self.cluster_size))
        self.log.info("%s/%s nodes terminated" % (len(terminated), self.cluster_size))
        
    def halt(self):
        if not self.has_started:
            self.log.warning("Cluster has not been created yet. Nothing to do")
            return
        q = queue.Queue()
        threads = [ threading.Thread(target=node.stop_instance) for node in new_nodes]
        for thread in threads:
            thread.start()
            thread.join()

    def terminate(self):
        if not self.has_started:
            self.log.warning("Cluster has not been created yet. Nothing to do")
            return
        q = queue.Queue()
        threads = [ threading.Thread(target=node.terminate_instance) for node in new_nodes]
        for thread in threads:
            thread.start()
            thread.join()

        self.has_started = False
        self.cluster_size = 0
        
    def start(self):
        if self.has_started:
            self.log.warning("Cluster has already started. Nothing to do")
            return
        om_node = ec2.EC2(name=name, is_om_node=True)
        om.node.start_instance()

        self.om_ip_nodes = [om_node]
        self.nodes = [om_node]

        for i in xrange(1, self.cluster_size):
            node = ec2.EC2(name=name)
            node.start_instance()
            self.nodes.append(node)
        
        self.has_started = True
        
    def create_ec2_single_increment(self, increment):
        '''
        Create a single node, and append each time, and append such node to the cluster
        Have to check for errors in case a node failed.
        '''
        assert increment >= 1
        for i in xrange(0, increment):
            node = ec2.EC2(name=name)
            node.start_instance()
            self.nodes.append(node)
        self.cluster_size += increment
        
    def create_ec2_multiple_increment(self, increment):
        assert increment >= 1
        new_nodes = []
        # create increment number of nodes
        for i in xrange(0, increment):
            node = ec2.EC2(name=self.name)
            new_nodes.apppend(node)
        
        # create the new nodes in parallel
        q = queue.Queue()
        threads = [ threading.Thread(target=node.start_instance) for node in new_nodes]
        for thread in threads:
            thread.start()
            thread.join()
        
        self.nodes.extend(new_nodes)
        self.cluster_size += increment
        
##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: ec2.py                                                               #
##############################################################################
import concurrent.futures
import logging
import random
import time

import ec2
import ansible_deploy

class Domain(object):

    '''
    Responsible for creating the FDS cluster. User can scale up or down the number of nodes,
    terminate or halt them.

    Attributes:
    -----------
    name: str
        the name of the cluster
    cluster_size: int
        the number of nodes created. Set the minimum to 1, default to 4
    '''
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)

    def __init__(self, name, cluster_size=4):

        assert cluster_size >= 1

        self.name = name
        # create the first single OM node
        om_node = ec2.EC2(name=name, is_om_node=True)

        # start the node
        self.__run_threads([om_node.start_instance])

        self.om_nodes = [om_node]
        self.nodes = [om_node]

        self.cluster_size = 1
        self.create_ec2_multiple_increment(cluster_size-1)
        self.has_started = True

    def check_status(self):
        '''
        Check the status of each node in the cluster.
        '''
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

        self.log.info("{}/{} nodes running".format(len(running), self.cluster_size))
        self.log.info("{}/{} nodes halted".format(len(halted), self.cluster_size))
        self.log.info("{}/{} nodes terminated".format(len(terminated), self.cluster_size))

    def resume(self):
        '''
        Check if the cluster is halted (stopped). If it is, then
        resume and bring the nodes up and running again.
        '''
        if self.has_started:
            self.log.warning("Cluster has not been created yet. Nothing to do")
            return

        targets = [ node.resume_instance for node in self.nodes ]
        self.__run_threads(targets)
        self.has_started = True
        self.check_status()

    def __run_threads(self, targets):
        '''
        Private method for running things in parallel.The target (method) is passed as an object
        '''
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            future_job = { executor.submit(target): target for target in targets }
            for future in concurrent.futures.as_completed(future_job):
                try:
                    data = future.result()
                except Exception as exc:
                    self.log.exception(exc)
                else:
                    continue

    def halt(self):
        '''
        Check if the cluster is running. If it is, then halt (stop) the nodes. The user can
        resume and bring the nodes up and running again. This will not destroy the nodes.
        '''
        if not self.has_started:
            self.log.warning("Cluster has not been created yet. Nothing to do")
            return

        targets = [ node.stop_instance for node in self.nodes ]
        self.__run_threads(targets)
        self.has_started = False
        self.check_status()

    def terminate(self):
        '''
        Check if the cluster is running. If it is, then
        destroy the existing nodes and any volumes attached to them.
        '''
        if not self.has_started:
            self.log.warning("Cluster has not been created yet. Nothing to do")
            return

        targets = [ node.terminate_instance for node in self.nodes ]
        self.__run_threads(targets)

        self.has_started = False
        self.cluster_size = 0
        self.check_status()

    def create_ec2_single_increment(self, increment):
        '''
        Add increment number of new nodes, one each time, and append such node to the cluster
        Have to check for errors in case a node failed.

        Arguments:
        ----------
        increment: int
            how many additional nodes to be added to the cluster.
        '''
        assert len(self.om_nodes) > 0
        om_node = self.om_nodes[0]
        for i in xrange(0, increment):
            node = ec2.EC2(name=self.name)
            node.start_instance()
            ansible_deploy.run_playbook(node.instance.ip_address, om_node)
            self.nodes.append(node)
        self.cluster_size += increment
        self.check_status()

    def create_ec2_multiple_increment(self, increment):
        '''
        Add increment number of new nodes, which are scale up in parallel for efficiency.
        Have to check for errors in case a node failed.

        Arguments:
        ----------
        increment: int
            how many additional nodes to be added to the cluster.
        '''
        new_nodes = []
        # create increment number of nodes
        for i in xrange(0, increment):
            node = ec2.EC2(name=self.name)
            new_nodes.append(node)

        # create the new nodes in parallel
        targets = [ node.start_instance for node in new_nodes ]
        self.__run_threads(targets)

        self.nodes.extend(new_nodes)
        self.cluster_size += increment
        self.check_status()

        assert len(self.om_nodes) > 0
        # later replace by a better way to group nodes by multiOM
        om_node = self.om_nodes[0]
        for node in self.nodes:
            ansible_deploy.run_playbook(node.instance.ip_address, om_node)

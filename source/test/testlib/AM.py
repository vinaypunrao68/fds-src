from __future__ import with_statement
from fabric import tasks
from fabric.api import *
from fabric.contrib.console import confirm
from fabric.contrib.files import *
from fabric.network import disconnect_all
from fabric.tasks import execute
import string
import pdb
from StringIO import StringIO
import re
import fabric_helper
import logging
import random
import time
import string

from fds.services.node_service import NodeService
from fds.services.fds_auth import FdsAuth
from fds.services.users_service import UsersService
from fds.model.node_state import NodeState
from fds.model.domain import Domain

#Need to get this from config.py file
env.user='root'
env.password='passwd'

random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

class am_service(object):
    def __init__(self):
        self.fds_log = '/tmp'
        fdsauth = FdsAuth()
        fdsauth.login()
	pdb.set_trace()
        self.nservice = NodeService(fdsauth)
        self.node_list = self.nservice.list_nodes()
        self.node_state = NodeState()
	    #TODO:  Get OM IP address from ./fdscli.conf/config.py file
        om_ip = "10.3.38.2"
        for node in self.node_list:
            if node.ip_v4_address == om_ip:
                env.host_string = node.ip_v4_address

    def start(self, node_ip):
        '''
        Start AM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start AM service.

	    Returns:
	    -----------
	    Boolean
        '''
        log.info(am_service.start.__name__)
        nodeNewState = NodeState()
        nodeNewState.am=True

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
		pdb.set_trace()
                self.nservice.start_service(node.id, node.services['AM'][0].id)
                if node.services['AM'][0].status ==  'ACTIVE':
			         log.info('AM service has started on node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('AM service has NOT started on node {}'.format(node.ip_v4_address))
			         return False 


    def stop(self, node_ip):
        '''
        Stop AM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to stop AM service.

    	Returns:
    	-----------
    	Boolean

        '''
        log.info(am_service.stop.__name__)
        nodeNewState = NodeState()
        nodeNewState.am=False

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.deactivate_node(node.id, nodeNewState)

                if node.services['AM'][0].status !=  'ACTIVE':
			         log.info('AM service is no longer running on node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('AM service is STILL running on node {}'.format(node.ip_v4_address))
			         return False


    def kill(self, node_ip):
        '''
        Kill AM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to kill AM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(am_service.kill.__name__)
        log.info('Killing bare_am service')
        env.host_string = node_ip
        sudo('pkill -9 bare_am > {}/cli.out 2>&1'.format(self.fds_log))

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                if node.services['AM'][0].status !=  'ACTIVE':
		        log.warn('Failed to kill bare_am service on node {}'.format(node.ip_v4_address))
		        return False 

		else:
			log.info('killed bare_am service on node {}'.format(node.ip_v4_address))
		        return True

    def add(self, node_ip):
        '''
        Add AM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to add AM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(am_service.add.__name__)
        log.info('Adding AM service')
        env.host_string = node_ip

	#TODO:  add code to add AM service here
        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                if node.services['AM'][0].status ==  'ACTIVE':
			         log.info('Added AM service to node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('Failed to add AM service to node {}'.format(node.ip_v4_address))
			         return False 

    def remove(self, node_ip):
        '''
        Remove AM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to remove AM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(am_service.add.__name__)
        log.info('Removing AM service')
        env.host_string = node_ip

	#TODO:  add code to remove AM service here
        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                if node.services['AM'][0].status ==  'ACTIVE':
			         log.info('Removed AM service from node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('Failed to remove AM service from node {}'.format(node.ip_v4_address))
			         return False 


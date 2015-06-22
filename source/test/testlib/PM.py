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
import sys

from fds.services.node_service import NodeService
from fds.services.fds_auth import FdsAuth
from fds.services.users_service import UsersService
from fds.model.node_state import NodeState
from fds.model.service import Service 
from fds.model.domain import Domain

sys.path.insert(0, '../../scale-framework')
import config

env.user=config.SSH_USER
env.password=config.SSH_PASSWORD

random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

class PMService(object):
    def __init__(self):
        self.fds_log = '/tmp'
        fdsauth = FdsAuth()
        fdsauth.login()
        self.nservice = NodeService(fdsauth)
        self.node_list = self.nservice.list_nodes()
        self.node_state = NodeState()
        om_ip = config.FDS_DEFAULT_HOST

        for node in self.node_list:
            if node.ip_v4_address == om_ip:
                env.host_string = node.ip_v4_address

    def start(self, node_ip):
        '''
        Start platformd service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start platformd service.

	    Returns:
	    -----------
	    Boolean
        '''
        log.info(PMService.start.__name__)
        nodeNewState = NodeState()
        nodeNewState.am=True

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.start_service(node.id, node.services['PM'][0].id)
                time.sleep(7)

        #check updated node state
        node_list = self.nservice.list_nodes()
        for node in node_list:
            if node.ip_v4_address == node_ip:
                if node.services['PM'][0].status ==  'ACTIVE':
			         log.info('PASS - PM service has started on node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('FAIL - PM service has NOT started on node {}'.format(node.ip_v4_address))
			         return False 


    def stop(self, node_ip):
        '''
        Stop PM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to stop PM service.

    	Returns:
    	-----------
    	Boolean

        '''
        log.info(PMService.stop.__name__)
        nodeNewState = NodeState()
        nodeNewState.am=False

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.stop_service(node.id, node.services['PM'][0].id)
                time.sleep(7)

        #check updated node state
        node_list = self.nservice.list_nodes()
        for node in node_list:
            if node.ip_v4_address == node_ip:
                if node.services['PM'][0].status !=  'ACTIVE':
			         log.info('PASS - PM service is no longer running on node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('FAIL - PM service is STILL running on node {}'.format(node.ip_v4_address))
			         return False


    def kill(self, node_ip):
        '''
        Kill PM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to kill PM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(PMService.kill.__name__)
        log.info('Killing platformd service')
        env.host_string = node_ip
        sudo('pkill -9 platformd')
	time.sleep(7)
	
	#get new node state
        node_list = self.nservice.list_nodes()
        for node in node_list:
            if node.ip_v4_address == node_ip:
                if node.services['PM'][0].status ==  'ACTIVE':
		        log.warn('FAIL - Failed to kill StorMgr service on node {}'.format(node.ip_v4_address))
		        return False 

		else:
			log.info('PASS - StorMgr service has been killed on node {}'.format(node.ip_v4_address))
		        return True

    def add(self, node_ip):
        '''
        Add PM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to add PM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(PMService.add.__name__)
        log.info('Adding PM service')
	fdsauth2 = FdsAuth()
	fdsauth2.login()
        newNodeService = Service()
	newNodeService.auto_name="PM"
	newNodeService.type="FDSP_STOR_MGR"
	newNodeService.status="ACTIVE"

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.add_service(node.id, newNodeService)
                time.sleep(7)

        #check updated node state
        node_list = self.nservice.list_nodes()
        for node in node_list:
            if node.ip_v4_address == node_ip:
                if node.services['PM'][0].status ==  'ACTIVE':
			         log.info('PASS - Added PM service to node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('FAIL - Failed to add PM service to node {}'.format(node.ip_v4_address))
			         return False 

    def remove(self, node_ip):
        '''
        Remove PM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to remove PM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(PMService.remove.__name__)
        log.info('Removing platformd service')
        nodeNewState = NodeState()
        nodeNewState.am=False

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.remove_service(node.id, node.services['PM'][0].id)
                time.sleep(7)

        #check updated node state
        node_list = self.nservice.list_nodes()
        for node in node_list:
            if node.ip_v4_address == node_ip:
                if node.services['PM'][0].status ==  'INACTIVE':
			         log.info('PASS - Removed PM service from node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('FAIL - Failed to remove platformd service from node {}'.format(node.ip_v4_address))
			         return False 


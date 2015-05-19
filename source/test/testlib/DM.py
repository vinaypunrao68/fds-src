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

#Need to get this from config.py file
env.user=config.SSH_USER
env.password=config.SSH_PASSWORD

random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

class dm_service(object):
    def __init__(self):
        self.fds_log = '/tmp'
        fdsauth = FdsAuth()
        fdsauth.login()
        self.nservice = NodeService(fdsauth)
        self.node_list = self.nservice.list_nodes()
        self.node_state = NodeState()
	    #TODO:  Get OM IP address from ./fdscli.conf/config.py file
        om_ip = config.FDS_DEFAULT_HOST
 
        for node in self.node_list:
            if node.ip_v4_address == om_ip:
                env.host_string = node.ip_v4_address

    def start(self, node_ip):
        '''
        Start DM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start DM service.

	    Returns:
	    -----------
	    Boolean
        '''
        log.info(dm_service.start.__name__)
        nodeNewState = NodeState()
        nodeNewState.am=True

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.start_service(node.id, node.services['DM'][0].id)
                if node.services['DM'][0].status ==  'ACTIVE':
			         log.info('DM service has started on node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('DM service has NOT started on node {}'.format(node.ip_v4_address))
			         return False 


    def stop(self, node_ip):
        '''
        Stop DM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to stop DM service.

    	Returns:
    	-----------
    	Boolean

        '''
        log.info(dm_service.stop.__name__)
        nodeNewState = NodeState()
        nodeNewState.am=False

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.stop_service(node.id, node.services['DM'][0].id)

                if node.services['DM'][0].status !=  'ACTIVE':
			         log.info('DM service is no longer running on node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('DM service is STILL running on node {}'.format(node.ip_v4_address))
			         return False


    def kill(self, node_ip):
        '''
        Kill DM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to kill DM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(dm_service.kill.__name__)
        log.info('Killing DataMgr service')
        env.host_string = node_ip
        #sudo('pkill -9 DataMgr > {}/cli.out 2>&1'.format(self.fds_log))
        sudo('pkill -9 DataMgr')

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                if node.services['DM'][0].status !=  'ACTIVE':
		        log.warn('Failed to kill DataMgr service on node {}'.format(node.ip_v4_address))
		        return False 

		else:
			log.info('killed DataMgr service on node {}'.format(node.ip_v4_address))
		        return True

    def add(self, node_ip):
        '''
        Add DM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to add DM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(dm_service.add.__name__)
        log.info('Adding DM service')
	fdsauth2 = FdsAuth()
	fdsauth2.login()
        newNodeService = Service()
	newNodeService.auto_name="DM"
	newNodeService.type="FDSP_DATA_MGR"
	newNodeService.status="ACTIVE"

	#TODO:  add code to add DM service here
        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.add_service(node.id, newNodeService)

                if node.services['DM'][0].status ==  'ACTIVE':
			         log.info('Added DM service to node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('Failed to add DM service to node {}'.format(node.ip_v4_address))
			         return False 

    def remove(self, node_ip):
        '''
        Remove DM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to remove DM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(dm_service.remove.__name__)
        log.info('Removing DM service')
        nodeNewState = NodeState()
        nodeNewState.am=False

	#TODO:  add code to remove DM service here
        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.remove_service(node.id, node.services['DM'][0].id)
                if node.services['DM'][0].status ==  'INACTIVE':
			         log.info('Removed DM service from node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('Failed to remove DM service from node {}'.format(node.ip_v4_address))
			         return False 


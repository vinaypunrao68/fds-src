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

class dm_service(object):
    def __init__(self):
        self.fds_log = '/tmp'
        fdsauth = FdsAuth()
        fdsauth.login()
        self.nservice = NodeService(fdsauth)
        self.node_list = self.nservice.list_nodes()
        self.node_state = NodeState()
	#TODO:  Get OM IP address fro ./fdscli.conf file
        om_ip = "10.3.82.40"
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
        nodeNewState.dm=True

        for node in self.node_list:
	    #need to comment out the line below when done
	    log.info('Node = {}, Status = {}'.format(node.ip_v4_address, node.services['DM'][0].status))
            if node.ip_v4_address == node_ip:
                self.nservice.activate_node(node.id, nodeNewState)
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
        nodeNewState.dm=False

        for node in self.node_list:
	    #need to comment out the line below when done
            log.info('Node = {}, Status = {}'.format(node.ip_v4_address, node.services['DM'][0].status))
            if node.ip_v4_address == node_ip:
                self.nservice.deactivate_node(node.id, nodeNewState)

                if node.services['DM'][0].status !=  'ACTIVE':
			log.info('DM service has stopped running on node {}'.format(node.ip_v4_address))
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
	pdb.set_trace()
	sudo('pkill -9 DataMgr > {}/cli.out 2>&1'.format(self.fds_log))

        for node in self.node_list:
	    #need to comment out the line below when done
	    log.info('Node = {}, Status = {}'.format(node.ip_v4_address, node.services['DM'][0].status))
            if node.ip_v4_address == node_ip:
                if node.services['DM'][0].status !=  'ACTIVE':
			log.info('killed DataMgr service on node {}'.format(node.ip_v4_address))
			return True

                else:
			log.warn('Failed to kill DataMgr service on node {}'.format(node.ip_v4_address))
			return False 


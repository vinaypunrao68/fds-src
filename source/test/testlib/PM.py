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
#import fabric_helper
import logging
import random
import time
import string
import sys

from fdscli.services.node_service import NodeService
from fdscli.services.fds_auth import FdsAuth
from fdscli.services.users_service import UsersService
#from fdscli.model.platform.node_state import NodeState
from fdscli.model.platform.node import Node
from fdscli.model.platform.service import Service 
from fdscli.model.platform.domain import Domain

#sys.path.insert(0, '../../scale-framework')
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
        self.node_state = Node()
        om_ip = config.FDS_DEFAULT_HOST
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == om_ip:
                env.host_string = eachnode.address.ipv4address

    def start(self, node_ip):
        '''
        Start PM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start PM service.

	    Returns:
	    -----------
	    Boolean
        '''
        log.info(PMService.start.__name__)
        #nodeNewState = NodeState()
        nodeNewState = Node()
        nodeNewState.pm=True

        #start PM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                #self.nservice.start_service(eachnode.id, eachnode.services['PM'][0].id)
                sudo('service fds-pm start')
                time.sleep(7)
                env.host_string = node_ip
                pm_pid = sudo('pgrep platformd')

        #check updated node state 
        node_list = self.nservice.list_nodes()
        try:
            for eachnode in node_list:
                if eachnode.address.ipv4address== node_ip:
                    if (eachnode.services['PM'][0].status.state ==  'ACTIVE' or eachnode.services['PM'][0].status.state ==  'RUNNING') and int(pm_pid) > 0:
                         log.info('PASS - PM service has started on node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - PM service has NOT started on node {}'.format(eachnode.address.ipv4address))
                         return False 

        except NameError:
            log.warn('FAIL = PM service has not started  - unable to locate node.')
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
        env.host_string = node_ip
        pm_pid_before = sudo('pgrep platformd')
        #nodeNewState = NodeState()
        nodeNewState = Node()
        nodeNewState.pm=False
        NodeFound = False

        #stop PM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                #self.nservice.stop_service(eachnode.id, eachnode.services['PM'][0].id)
                sudo('service fds-pm stop')
                time.sleep(7)
                NodeFound = True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    if eachnode.services['PM'][0].status.state ==  'INACTIVE' or eachnode.services['PM'][0].status.state ==  'DOWN' or eachnode.services['PM'][0].status.state ==  'NOT_RUNNING':
                         log.info('PASS - PM service is no longer running on node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - PM service is STILL running on node {}'.format(eachnode.address.ipv4address))
                         return False

        else:
            log.warn('Fail - PM service has not started - unable to locate node')
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
        env.host_string = node_ip
        pm_pid_before = sudo('pgrep platformd')

        log.info(PMService.kill.__name__)
        log.info('Killing platformd service')
        sudo('pkill -9 platformd')
        time.sleep(7)
        pm_pid_after = sudo('pgrep platformd')
        #Get new node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                #if eachnode.services['PM'][0].status ==  'ACTIVE':
                if pm_pid_before ==  pm_pid_after:
		        log.warn('FAIL - Failed to kill platformd service on node {}'.format(eachnode.address.ipv4address))
		        return False 

		else:
			log.info('PASS - platformd service has been killed on node {}'.format(eachnode.address.ipv4address))
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
    	#newNodeService.type="FDSP_ACCESS_MGR"
        newNodeService.type="PM"
        #newNodeService.status="ACTIVE"
        #newNodeService.state="UP"
        newNodeService.status.state="NOT_RUNNING"

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.add_service(eachnode.id, newNodeService)

		time.sleep(7)
        env.host_string = node_ip

	    #check updated node state 
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                if eachnode.services['PM'][0].status.state ==  'NOT_RUNNING':
			         log.info('PASS - Added PM service to node {}'.format(eachnode.address.ipv4address))
			         return True

                else:
			         log.warn('FAIL - Failed to add PM service to node {}'.format(eachnode.address.ipv4address))
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
        log.info('Removing PM service')
        #nodeNewState = NodeState()
        nodeNewState = Node()
        nodeNewState.pm=False
        NodeFound = False

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.remove_service(eachnode.id, eachnode.services['PM'][0].id)
                time.sleep(7)
                NodeFound = True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['PM'][0].status.state ==  'INACTIVE' or \
                            eachnode.services['PM'][0].status.state ==  'NOT_RUNNING' or \
                            eachnode.services['PM'][0].status.state == 'RUNNING':
                             log.warn('FAIL - Failed to remove PM service from node {}'.format(eachnode.address.ipv4address))
                             return False 

                    except IndexError:
                             log.info('PASS - Removed PM service from node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('FAIL - PM service has not been removed - unable to locate node')
            return False

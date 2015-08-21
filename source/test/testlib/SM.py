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

class SMService(object):
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
        Start SM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start SM service.

	    Returns:
	    -----------
	    Boolean
        '''
        log.info(SMService.start.__name__)
        #nodeNewState = NodeState()
        nodeNewState = Node()
        nodeNewState.sm=True

        #start SM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.start_service(eachnode.id, eachnode.services['SM'][0].id)
                time.sleep(7)
                env.host_string = node_ip
                sm_pid = sudo('pgrep StorMgr')

        #check updated node state 
        node_list = self.nservice.list_nodes()
        try:
            for eachnode in node_list:
                if eachnode.address.ipv4address== node_ip:
                    #if (eachnode.services['SM'][0].status.state ==  'ACTIVE' or eachnode.services['SM'][0].status.state ==  'RUNNING') and int(sm_pid) > 0:
                    if (eachnode.services['SM'][0].status.state ==  'RUNNING'):
                         log.info('PASS - SM service has started on node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - SM service has NOT started on node {}'.format(eachnode.address.ipv4address))
                         return False 

        except NameError:
            log.warn('FAIL = SM service has not started  - unable to locate node.')
            return False
                

    def stop(self, node_ip):
        '''
        Stop SM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to stop SM service.

    	Returns:
    	-----------
    	Boolean

        '''
        log.info(SMService.stop.__name__)
        env.host_string = node_ip
        nodeNewState = Node()
        nodeNewState.sm=False
        NodeFound = False

        #stop SM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.stop_service(eachnode.id, eachnode.services['SM'][0].id)
                time.sleep(7)
                NodeFound = True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['SM'][0].status.state ==  'INACTIVE' or eachnode.services['SM'][0].status.state ==  'DOWN' or eachnode.services['SM'][0].status.state ==  'NOT_RUNNING':
                             log.info('PASS - SM service is no longer running on node {}'.format(eachnode.address.ipv4address))
                             return True

                        else:
                             log.warn('FAIL - SM service is STILL running on node {}'.format(eachnode.address.ipv4address))
                             return False
                
                    except IndexError:
                        log.warn('WARNING - SM service is not available on node {}'.format(eachnode.address.ipv4address))
                        return True

        else:
            log.warn('Fail - SM service has not started - unable to locate node')
            return False

    def kill(self, node_ip):
        '''
        Kill SM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to kill SM service.

    	Returns:
    	-----------
    	Boolean
        '''
        env.host_string = node_ip
        sm_pid_before = sudo('pgrep StorMgr')

        log.info(SMService.kill.__name__)
        log.info('Killing StorMgr service')
        sudo('pkill -9 StorMgr')
        time.sleep(7)
        sm_pid_after = sudo('pgrep StorMgr')
        #Get new node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                #if eachnode.services['SM'][0].status ==  'ACTIVE':
                if sm_pid_before ==  sm_pid_after:
		        log.warn('FAIL - Failed to kill StorMgr service on node {}'.format(eachnode.address.ipv4address))
		        return False 

		else:
			log.info('PASS - StorMgr service has been killed on node {}'.format(eachnode.address.ipv4address))
		        return True

    def add(self, node_ip):
        '''
        Add SM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to add SM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(SMService.add.__name__)
        log.info('Adding SM service')
        fdsauth2 = FdsAuth()
        fdsauth2.login()
        newNodeService = Service()
        newNodeService.auto_name="SM"
    	#newNodeService.type="FDSP_ACCESS_MGR"
        newNodeService.type="SM"
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
                try:
                    if eachnode.services['SM'][0].status.state ==  'NOT_RUNNING':
                         log.info('PASS - Added SM service to node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - Failed to add SM service to node {}'.format(eachnode.address.ipv4address))
                         return False

                except IndexError:
                    log.warn('FAIL - Failed to add SM service to node {}'.format(eachnode.address.ipv4address))
                    return False



    def remove(self, node_ip):
        '''
        Remove SM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to remove SM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(SMService.remove.__name__)
        log.info('Removing SM service')
        #nodeNewState = NodeState()
        nodeNewState = Node()
        nodeNewState.sm=False
        NodeFound = False

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    self.nservice.remove_service(eachnode.id, eachnode.services['SM'][0].id)
                    time.sleep(7)
                    NodeFound = True

                except IndexError:
                    log.warn('WARNING - SM service is not available on node {}'.format(eachnode.address.ipv4address))
                    return True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['SM'][0].status.state ==  'INACTIVE' or \
                            eachnode.services['SM'][0].status.state ==  'NOT_RUNNING' or \
                            eachnode.services['SM'][0].status.state == 'RUNNING':
                             log.warn('FAIL - Failed to remove SM service from node {}'.format(eachnode.address.ipv4address))
                             return False 

                    except IndexError:
                             log.info('PASS - Removed SM service from node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('FAIL - SM service has not been removed - unable to locate node')
            return False

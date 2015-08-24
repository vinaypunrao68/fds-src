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

class DMService(object):
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
        Start DM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start DM service.

	    Returns:
	    -----------
	    Boolean
        '''
        log.info(DMService.start.__name__)
        #nodeNewState = NodeState()
        nodeNewState = Node()
        nodeNewState.dm=True

        #start DM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.start_service(eachnode.id, eachnode.services['DM'][0].id)
                time.sleep(7)
                env.host_string = node_ip
                dm_pid = sudo('pgrep DataMgr')

        #check updated node state 
        node_list = self.nservice.list_nodes()
        try:
            for eachnode in node_list:
                if eachnode.address.ipv4address== node_ip:
                    #if (eachnode.services['DM'][0].status.state ==  'ACTIVE' or eachnode.services['DM'][0].status.state ==  'RUNNING') and int(dm_pid) > 0:
                    if (eachnode.services['DM'][0].status.state ==  'RUNNING'):
                         log.info('PASS - DM service has started on node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - DM service has NOT started on node {}'.format(eachnode.address.ipv4address))
                         return False 

        except NameError:
            log.warn('FAIL = DM service has not started  - unable to locate node.')
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
        log.info(DMService.stop.__name__)
        env.host_string = node_ip
        nodeNewState = Node()
        nodeNewState.dm=False
        NodeFound = False

        #stop DM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.stop_service(eachnode.id, eachnode.services['DM'][0].id)
                time.sleep(7)
                NodeFound = True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['DM'][0].status.state ==  'INACTIVE' or eachnode.services['DM'][0].status.state ==  'DOWN' or eachnode.services['DM'][0].status.state ==  'NOT_RUNNING':
                             log.info('PASS - DM service is no longer running on node {}'.format(eachnode.address.ipv4address))
                             return True

                        else:
                             log.warn('FAIL - DM service is STILL running on node {}'.format(eachnode.address.ipv4address))
                             return False

                    except IndexError:
                        log.warn('WARNING - AM service is not available on node {}'.format(eachnode.address.ipv4add))
                        return True

        else:
            log.warn('Fail - DM service has not started - unable to locate node')
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
        env.host_string = node_ip
        dm_pid_before = sudo('pgrep DataMgr')

        log.info(DMService.kill.__name__)
        log.info('Killing DataMgr service')
        sudo('pkill -9 DataMgr')
        time.sleep(7)
        dm_pid_after = sudo('pgrep DataMgr')
        #Get new node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                #if eachnode.services['DM'][0].status ==  'ACTIVE':
                if dm_pid_before ==  dm_pid_after:
		        log.warn('FAIL - Failed to kill DataMgr service on node {}'.format(eachnode.address.ipv4address))
		        return False 

		else:
			log.info('PASS - DataMgr service has been killed on node {}'.format(eachnode.address.ipv4address))
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
        log.info(DMService.add.__name__)
        log.info('Adding DM service')
        fdsauth2 = FdsAuth()
        fdsauth2.login()
        newNodeService = Service()
        newNodeService.auto_name="DM"
    	#newNodeService.type="FDSP_ACCESS_MGR"
        newNodeService.type="DM"
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
                    if eachnode.services['DM'][0].status.state ==  'NOT_RUNNING':
                         log.info('PASS - Added DM service to node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - Failed to add DM service to node {}'.format(eachnode.address.ipv4address))
                         return False 

                except IndexError:
                    log.warn('FAIL - Failed to add DM service to node {}'.format(eachnode.address.ipv4address))
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
        log.info(DMService.remove.__name__)
        log.info('Removing DM service')
        #nodeNewState = NodeState()
        nodeNewState = Node()
        nodeNewState.dm=False
        NodeFound = False

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    self.nservice.remove_service(eachnode.id, eachnode.services['DM'][0].id)
                    time.sleep(7)
                    NodeFound = True

                except IndexError:
                    log.warn('WARNING - DM service is not available on node {}'.format(eachnode.address.ipv4address))
                    return True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['DM'][0].status.state ==  'INACTIVE' or \
                            eachnode.services['DM'][0].status.state ==  'NOT_RUNNING' or \
                            eachnode.services['DM'][0].status.state == 'RUNNING':
                             log.warn('FAIL - Failed to remove DM service from node {}'.format(eachnode.address.ipv4address))
                             return False 

                    except IndexError:
                             log.info('PASS - Removed DM service from node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('FAIL - DM service has not been removed - unable to locate node')
            return False

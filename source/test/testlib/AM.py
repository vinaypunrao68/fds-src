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

class AMService(object):
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
        Start AM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start AM service.

	    Returns:
	    -----------
	    Boolean
        '''
        log.info(AMService.start.__name__)
        #nodeNewState = NodeState()
        nodeNewState = Node()
        nodeNewState.am=True

        #start AM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.start_service(eachnode.id, eachnode.services['AM'][0].id)
                time.sleep(7)
                env.host_string = node_ip
                am_pid = sudo('pgrep bare_am')

        #check updated node state 
        node_list = self.nservice.list_nodes()
        try:
            for eachnode in node_list:
                if eachnode.address.ipv4address== node_ip:
                    #if (eachnode.services['AM'][0].status.state ==  'ACTIVE' or eachnode.services['AM'][0].status.state ==  'RUNNING') and int(am_pid) > 0:
                    if (eachnode.services['AM'][0].status.state ==  'RUNNING'):
                         log.info('PASS - AM service has started on node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - AM service has NOT started on node {}'.format(eachnode.address.ipv4address))
                         return False 

        except NameError:
            log.warn('FAIL = AM service has not started  - unable to locate node.')
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
        log.info(AMService.stop.__name__)
        env.host_string = node_ip
        nodeNewState = Node()
        nodeNewState.am=False
        NodeFound = False

        #stop AM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.stop_service(eachnode.id, eachnode.services['AM'][0].id)
                time.sleep(7)
                NodeFound = True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['AM'][0].status.state ==  'INACTIVE' or eachnode.services['AM'][0].status.state ==  'DOWN' or eachnode.services['AM'][0].status.state ==  'NOT_RUNNING':
                             log.info('PASS - AM service is no longer running on node {}'.format(eachnode.address.ipv4address))
                             return True

                        else:
                             log.warn('FAIL - AM service is STILL running on node {}'.format(eachnode.address.ipv4address))
                             return False

                    except IndexError:
                             log.warn('WARNING - AM service is not available on node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('Fail - AM service has not started - unable to locate node')
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
        env.host_string = node_ip
        am_pid_before = sudo('pgrep bare_am')

        log.info(AMService.kill.__name__)
        log.info('Killing bare_am service')
        sudo('pkill -9 bare_am')
        time.sleep(7)
        am_pid_after = sudo('pgrep bare_am')
        #Get new node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                #if eachnode.services['AM'][0].status ==  'ACTIVE':
                if am_pid_before ==  am_pid_after:
		        log.warn('FAIL - Failed to kill bare_am service on node {}'.format(eachnode.address.ipv4address))
		        return False 

		else:
			log.info('PASS - bare_am service has been killed on node {}'.format(eachnode.address.ipv4address))
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
        log.info(AMService.add.__name__)
        log.info('Adding AM service')
        fdsauth2 = FdsAuth()
        fdsauth2.login()
        newNodeService = Service()
        newNodeService.auto_name="AM"
    	#newNodeService.type="FDSP_ACCESS_MGR"
        newNodeService.type="AM"
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
                    if eachnode.services['AM'][0].status.state ==  'NOT_RUNNING':
                         log.info('PASS - Added AM service to node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - Failed to add AM service to node {}'.format(eachnode.address.ipv4address))
                         return False 

                except IndexError:
                    log.warn('FAIL - Failed to add AM service to node {}'.format(eachnode.address.ipv4address))
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
        log.info(AMService.remove.__name__)
        log.info('Removing AM service')
        #nodeNewState = NodeState()
        nodeNewState = Node()
        nodeNewState.am=False
        NodeFound = False

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    self.nservice.remove_service(eachnode.id, eachnode.services['AM'][0].id)
                    time.sleep(7)
                    NodeFound = True

                except IndexError:
                         log.warn('WARNING - AM service is not available on node {}'.format(eachnode.address.ipv4address))
                         return True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['AM'][0].status.state ==  'INACTIVE' or \
                            eachnode.services['AM'][0].status.state ==  'NOT_RUNNING' or \
                            eachnode.services['AM'][0].status.state == 'RUNNING':
                             log.warn('FAIL - Failed to remove AM service from node {}'.format(eachnode.address.ipv4address))
                             return False 

                    except IndexError:
                             log.info('PASS - Removed AM service from node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('FAIL - AM service has not been removed - unable to locate node')
            return False

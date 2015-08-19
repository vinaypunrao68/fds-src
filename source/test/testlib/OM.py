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
#import fabric_helper as fh
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

class OMService(object):
    '''
    NOTE:  This class communicates to the main OM to start/stop/kill
    an OM service on another node.  It does not kill the OM process
    on the main OM node it talks to.
    
    Attributes:
    -----------
    om_ip in the init is the main IP of an OM node
    '''
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
        Start OM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start OM service.

	    Returns:
	    -----------
	    Boolean
        '''
        log.info(OMService.start.__name__)
        #fhObj = fh.FabricHelper(node_ip)
        env.host_string = node_ip
        log.info('Starting OM service...')

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                #self.nservice.start_service(eachnode.id, eachnode.services['OM'][0].id)
                sudo('service fds-om start')
                time.sleep(7)

        #check updated node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                if eachnode.services['OM'][0].status.state ==  'ACTIVE' or eachnode.services['OM'][0].status.state ==  'RUNNING':
			         log.info('PASS - OM service has started on node {}'.format(eachnode.address.ipv4address))
			         return True

                else:
			         log.warn('FAIL - OM service has NOT started on node {}'.format(eachnode.address.ipv4address))
			         return False 


    def stop(self, node_ip):
        '''
        Stop OM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node (non main OM) to stop OM service.

    	Returns:
    	-----------
    	Boolean

        '''
        log.info(OMService.stop.__name__)
        env.host_string = node_ip
        om_pid_before = sudo("ps aux | grep om.Main | grep -v grep | awk '{print $2}'")
        #fhObj = fh.FabricHelper(node_ip)
        log.info('Stopping OM service...')

        #sudo('service fds-om stop')
        #cmd_output = sudo('service fds-om status')
        #if cmd_output.find('om stop') > 0:
		#	         log.info('OM service is no longer running on node {}'.format(node_ip))
		#	         return True

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                #self.nservice.stop_service(eachnode.id, eachnode.services['OM'][0].id)
                sudo('service fds-om stop')
                time.sleep(7)
                #om_pid_after = sudo('pgrep java')
                try:
                    om_pid_after = sudo("ps aux | grep om.Main | grep -v grep | awk '{print $2}'")

                except SystemExit:
                    log.info('stopping OM service on node {}'.format(node_ip))

        #check updated node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                #if eachnode.services['OM'][0].status.state !=  'ACTIVE':
                if om_pid_before != om_pid_after:
			        log.info('PASS - OM service is no longer running on node {}'.format(eachnode.address.ipv4address))
			        return True

                else:
                    log.warn('FAIL - OM service is STILL running on node {}'.format(eachnode.address.ipv4address))
                    return False


    def kill(self, node_ip):
        '''
        Kill OM service

        Attributes:
        -----------
        node_ip:  str
        The IP address of the node to kill OM service.

        Returns:
        -----------
        Boolean
        '''
        log.info(OMService.kill.__name__)
        node_list = self.nservice.list_nodes()
        env.host_string = node_ip
        om_pid_before = sudo("ps aux | grep om.Main | grep -v grep | awk '{print $2}'")
        #fhObj = fh.FabricHelper(node_ip)
        #om_pid = fhObj.get_service_pid('om.Main')
        log.info('Killing OM service')
        #sudo('kill -9 {}'.format(om_pid))
        try:
            cmd_output = sudo('pkill -9 -f com.formationds.om.Main')
            time.sleep(7)

        except SystemExit:
            log.info('PASS - killed OM service on node {}'.format(node_ip))

        om_pid_after = sudo("ps aux | grep om.Main | grep -v grep | awk '{print $2}'")

        #cmd_output = sudo('service fds-om status')
        #if cmd_output.find('om stop') > 0:
        #	         log.info('OM service is no longer running on node {}'.format(node_ip))
        #	         return True


        #check updated node state
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
               try:
                   # if eachnode.services['OM'][0].status.state ==  'ACTIVE':
                    if om_pid_before == om_pid_after:
                        log.warn('FAIL - Failed to kill OM service on node {}'.format(eachnode.address.ipv4address))
                        return False 

                    else:
                        log.info('PASS - killed OM service on node {}'.format(eachnode.address.ipv4address))
                        return True

               except IndexError:
                    log.info('FAIL - killed OM service on node {}'.format(eachnode.address.ipv4address))
                    return True


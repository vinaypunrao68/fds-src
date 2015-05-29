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
import fabric_helper as fh
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
        self.node_state = NodeState()
        om_ip = config.FDS_DEFAULT_HOST
        for node in self.node_list:
            if node.ip_v4_address == om_ip:
                env.host_string = node.ip_v4_address

        

    def start(self, node_ip):
        '''
        Start OM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node (non main OM) to start OM service.

	    Returns:
	    -----------
	    Boolean
        '''
        log.info(OMService.start.__name__)
        fhObj = fh.FabricHelper(node_ip)
        log.info('Starting FDSP_ORCH_MGR service...')
        env.host_string = node_ip

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.start_service(node.id, node.services['OM'][0].id)

                if node.services['OM'][0].status !=  'ACTIVE':
			         log.info('OM service has started on node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('OM service has NOT started on node {}'.format(node.ip_v4_address))
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
        fhObj = fh.FabricHelper(node_ip)
        log.info('Stopping FDSP_ORCH_MGR service...')
        env.host_string = node_ip

        #sudo('service fds-om stop')
        #cmd_output = sudo('service fds-om status')
        #if cmd_output.find('om stop') > 0:
		#	         log.info('OM service is no longer running on node {}'.format(node_ip))
		#	         return True

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.stop_service(node.id, node.services['OM'][0].id)

                if node.services['OM'][0].status !=  'ACTIVE':
			         log.info('OM service is no longer running on node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('OM service is STILL running on node {}'.format(node.ip_v4_address))
			         return False


    def kill(self, node_ip):
        '''
        Kill OM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node (non main OM) to kill OM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(OMService.kill.__name__)
        fhObj = fh.FabricHelper(node_ip)
        om_pid = fhObj.get_service_pid('om.Main')
        log.info('Killing FDSP_ORCH_MGR service')
        env.host_string = node_ip
        sudo('kill -9 {}'.format(om_pid))

        #cmd_output = sudo('service fds-om status')
        #if cmd_output.find('om stop') > 0:
		#	         log.info('OM service is no longer running on node {}'.format(node_ip))
		#	         return True

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
               try:
                    if node.services['OM'][0].status ==  'ACTIVE':
                        log.warn('Failed to kill FDSP_ORCH_MGR service on node {}'.format(node.ip_v4_address))
                        return False 

                    else:
                        log.info('killed FDS_ORCH_MGR service on node {}'.format(node.ip_v4_address))
                        return True

               except IndexError:
                    log.info('killed FDS_ORCH_MGR service on node {}'.format(node.ip_v4_address))
                    return True


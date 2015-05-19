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

class pm_service(object):

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
        Start PM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start PM service.

	    Returns:
	    -----------
	    Boolean
        '''
        log.info(pm_service.start.__name__)
        nodeNewState = NodeState()
        nodeNewState.pm=True

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.start_service(node.id, node.services['PM'][0].id)
                if node.services['PM'][0].status ==  'ACTIVE':
			         log.info('PM service has started on node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('PM service has NOT started on node {}'.format(node.ip_v4_address))
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
        log.info(pm_service.stop.__name__)
        nodeNewState = NodeState()
        nodeNewState.pm=False

        for node in self.node_list:
            if node.ip_v4_address == node_ip:
                self.nservice.stop_service(node.id, node.services['PM'][0].id)

                if node.services['PM'][0].status !=  'ACTIVE':
			         log.info('PM service is no longer running on node {}'.format(node.ip_v4_address))
			         return True

                else:
			         log.warn('PM service is STILL running on node {}'.format(node.ip_v4_address))
			         return False



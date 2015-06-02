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
import time

sys.path.insert(0, '../../scale-framework')
import config

env.user=config.SSH_USER
env.password=config.SSH_PASSWORD


random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

class InfluxdbService(object):
    def __init__(self):
        om_ip = config.FDS_DEFAULT_HOST
        env.host_string = om_ip 

    def start(self, node_ip):
        '''
        Start Influxdb service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start Influxdb service.

	    Returns:
	    -----------
	    Boolean
        '''
	fhobj = fh.FabricHelper(node_ip)
	env.host_string = node_ip
        log.info(InfluxdbService.start.__name__)
	srv_id = fhobj.get_service_pid('influxdb')
	if srv_id == None:
		log.info('Starting Influxdb service on node {}'.format(node_ip))
		sudo('service influxdb start')	
		time.sleep(3)
		srv_id = fhobj.get_service_pid('influxdb')
		if srv_id != None:
			log.info('Influxdb service has started on node {}'.format(node_ip))
		        return True

                else:
			log.info('Failed to start Influxdb service on node {}'.format(node_ip))
		        return False

	else:
		log.info('Influxdb service is already running on node {}'.format(node_ip))
		log.info('There is nothing to do.')
		return True

    def stop(self, node_ip):
        '''
        Stop Influxdb service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to stop Influxdb service.

	    Returns:
	    -----------
	    Boolean
        '''
	fhobj = fh.FabricHelper(node_ip)
	env.host_string = node_ip
        log.info(InfluxdbService.stop.__name__)
	srv_id = fhobj.get_service_pid('influxdb')
	if srv_id != None:
		log.info('Stopping Influxdb service on node {}'.format(node_ip))
		sudo('service influxdb stop')	
		time.sleep(3)
		srv_id = fhobj.get_service_pid('influxdb')
		if srv_id == None:
			log.info('Influxdb service has stopped on node {}'.format(node_ip))
		        return True

                else:
			log.info('Failed to stop Influxdb service on node {}'.format(node_ip))
		        return False

	else:
		log.info('Influxdb service is already stopped on node {}'.format(node_ip))
		log.info('There is nothing to do.')
		return True

    def kill(self, node_ip):
        '''
        Kill Influxdb service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to kill Influxdb service.

	    Returns:
	    -----------
	    Boolean
        '''
	fhobj = fh.FabricHelper(node_ip)
	env.host_string = node_ip
        log.info(InfluxdbService.kill.__name__)
	srv_id = fhobj.get_service_pid('influxdb')
	if srv_id != None:
		log.info('Killing Influxdb process on node {}'.format(node_ip))
		sudo('kill -9 {}'.format(srv_id))	
		time.sleep(3)
		srv_id = fhobj.get_service_pid('influxdb')
		if srv_id == None:
			log.info('Influxdb service has been killed on node {}'.format(node_ip))
		        return True

                else:
			log.info('Failed to kill Influxdb service on node {}'.format(node_ip))
		        return False

	else:
		log.info('Influxdb service is not running on node {}'.format(node_ip))
		log.info('There is nothing to do.')
		return True


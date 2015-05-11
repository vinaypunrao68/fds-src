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

class am_service(object):
    def __init__(self):
		#self.fh = fabric_helper.FdsFabricHelper('AMAgent', am_ip)
        self.fds_log = '/tmp'
        self.fds_bin = '/fds/bin'
        self.fdsconsole= '/fds/sbin/fdsconsole.py'
        self.fds_sbin = '/fds/sbin'
        self.fdsconsole= '{}/fdsconsole.py'.format(self.fds_sbin)
        #self.fds_sbin= '/home/hlim/projects/fds-src/source/tools'
        #self.fds_sbin = '/home/hlim/projects/fds-src/source/tools'
        #self.am_pid = self.fh.get_service_pid()
        #self.am_uuid = self.fh.get_service_uuid(self.am_pid)
        self.am_port = 7000
        self.fds_dir = '/fds'
        fdsauth = FdsAuth()
        fdsauth.login()
        self.nservice = NodeService(fdsauth)
        self.node_list = self.nservice.list_nodes()
        self.node_state = NodeState()
        om_ip = "10.3.16.213"
        for node in self.node_list:
            if node.ip_v4_address == om_ip:
                env.host_string = node.ip_v4_address

    def start(self, node_ip):
        '''
        Start AM service
        '''
        nodeNewState = NodeState()
        nodeNewState.am=True
        for node in self.node_list:
            log.info('Node = {}, Status = {}'.format(node.ip_v4_address, node.services['AM'][0].status))
            if node.ip_v4_address == node_ip:
                self.nservice.activate_node(node.id, nodeNewState)
                log.info(am_service.start.__name__)
                if node.services['AM'][0].status ==  'ACTIVE':
                    log.info('AM service has started on node {}'.format(node.ip_v4_address))

                else:
                    log.warn('AM service has NOT started on node {}'.format(node.ip_v4_address))

    def stop(self, node_ip):
        '''
        Stop AM service
        '''
        nodeNewState = NodeState()
        nodeNewState.am=False
        for node in self.node_list:
            log.info('Node = {}, Status = {}'.format(node.ip_v4_address, node.services['AM'][0].status))
            if node.ip_v4_address == node_ip:
                self.nservice.deactivate_node(node.id, nodeNewState)
                log.info(am_service.stop.__name__)
                if node.services['AM'][0].status !=  'ACTIVE':
                    log.info('AM service has stopped running on node {}'.format(node.ip_v4_address))

                else:
                    log.warn('AM service is STILL running on node {}'.format(node.ip_v4_address))


    def kill(self, node_ip):
        '''
        Kill AM service
        '''
        log.info('Killing AMAgent service')
        cmd_status= sudo('pkill -9 bare_am')

        for node in self.node_list:
            log.info('Node = {}, Status = {}'.format(node.ip_v4_address, node.services['AM'][0].status))
            if node.ip_v4_address == node_ip:
                log.info(am_service.kill.__name__)
                if node.services['AM'][0].status !=  'ACTIVE':
                    log.info('killed bare_am service on node {}'.format(node.ip_v4_address))

                else:
                    log.warn('Failed to kill bare_am service on node {}'.format(node.ip_v4_address))

    #def remove(self):
	#	'''
	#	Remove AM service
	#	'''

	#	if exists('{}'.format(self.fdsconsole)):
	#		node_uuid = self.fh.get_node_uuid(env.host_string)
	#		print 'node_uuid = {}'.format(node_uuid)
	#		if node_uuid != None:
	#			with cd('{}'.format(self.fds_sbin)):
	#				sudo('./fdsconsole.py service removeService {} am > {}/cli.out 2>&1'.format(node_uuid, self.fds_log))
	#		else:
	#			log.warn('Unable to locate node_uuid for AMAgent service: node_uuid={}'.format(node_uuid))
	#	else:
	#		log.warn('Unable to locate {}'.format(self.fdsconsole))

    #def add(self):
	#	'''
	#	Add AM service
	#	'''

	#	if exists('{}'.format(self.fdsconsole)):
	#		node_uuid = self.fh.get_node_uuid(env.host_string)
	#		if node_uuid != None:
	#			with cd('{}'.format(self.fds_sbin)):
	#				sudo('./fdsconsole.py service addService {} am > {}/cli.out 2>&1'.format(node_uuid, self.fds_log))

	#		else:
	#			log.warn('Unable to locate node_uuid for AMAgent service: node_uuid={}'.format(node_uuid))
	#	else:
	#		log.warn('Unable to locate {}'.format(self.fdsconsole))

	#	with cd('{}'.format(self.fds_sbin)):
	#		#run('./fdsconsole.py service removeService {} am > {}/cli.out 2>&1'.format(self.am_uuid, self.fds_log))
	#		sudo('./fdsconsole.py service removeService {} am > cli.out 2>&1'.format(self.am_uuid))




#instance = am_service()
#am_util.add_class_methods_as_module_level_functions_for_fabric(instance, __name__)
#instance.special()

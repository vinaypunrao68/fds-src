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

#Need to get this from config.py file
env.user='root'
env.password='passwd'

random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

class am_service(object):
	
	def __init__(self, am_ip):
		env.host_string = am_ip
		self.fh = fabric_helper.FdsFabricHelper('AMAgent', am_ip)
		self.fds_log = '/tmp'
		self.fds_bin = '/fds/bin'
		#self.fdsconsole= '/fds/sbin/fdsconsole.py'
		#self.fds_sbin = '/fds/sbin'
		self.fdsconsole= '{}/fdsconsole.py'.format(self.fds_sbin)
		self.fds_sbin= '/home/hlim/projects/fds-src/source/tools'
		self.am_port = 7000
		self.fds_dir = '/fds'

	def start(self):
		'''
		Start AM service
		'''

		with cd('{}'.format(self.fds_sbin)):
			cmd_status = sudo('bash -c \"(nohup ./AMAgent --fds-root={} 0<&- &> {}/cli.out &) \"'.format(self.fds_dir, self.fds_log))

			if cmd_status.return_code == 0:
				log.info('AMAgent service started')

			else:
				log.warn('Failed to start AMAgent service: cmd_status={}'.format(cmd_status))
			

	def stop(self):
		'''
		Stop AM service
		'''
		#sudo('pkill -9 bare_am')
		output = sudo('ps auxw | grep -i amagent')
		if re.search('170483924568920640', output):
			print 'true'
			
	def kill(self):
		'''
		Kill AM service
		'''

		am_pid = self.fh.get_service_pid()
		if am_pid > 0:
			log.info('Killing AMAgent service')
			cmd_status= sudo('kill -9 {}'.format(am_pid))

			if cmd_status.return_code == 0:
				log.info('Successfully killed AMAgent service')

			else:
				log.warn('Failed to kill AMAgent service {}'.format(cmd_status))
			
		else:
			log.info('AMAgent service is not running.')

	def remove(self):
		'''
		Remove AM service
		'''
		
		if exists('{}'.format(self.fdsconsole)):
			node_uuid = self.fh.get_node_uuid(env.host_string)
			print 'node_uuid = {}'.format(node_uuid)
			if node_uuid != None:
				with cd('{}'.format(self.fds_sbin)):
					sudo('./fdsconsole.py service removeService {} am > {}/cli.out 2>&1'.format(node_uuid, self.fds_log))
			else:
				log.warn('Unable to locate node_uuid for AMAgent service: node_uuid={}'.format(node_uuid))
		else:
			log.warn('Unable to locate {}'.format(self.fdsconsole))
				
	def add(self):
		'''
		Add AM service
		'''

		if exists('{}'.format(self.fdsconsole)):
			node_uuid = self.fh.get_node_uuid(env.host_string)
			if node_uuid != None:
				with cd('{}'.format(self.fds_sbin)):
					sudo('./fdsconsole.py service addService {} am > {}/cli.out 2>&1'.format(node_uuid, self.fds_log))

			else:
				log.warn('Unable to locate node_uuid for AMAgent service: node_uuid={}'.format(node_uuid))
		else:
			log.warn('Unable to locate {}'.format(self.fdsconsole))
		


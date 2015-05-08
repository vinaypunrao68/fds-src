from __future__ import with_statement
from fabric import tasks
from fabric.api import *
from fabric.contrib.console import confirm
<<<<<<< HEAD
from fabric.contrib.files import *
=======
>>>>>>> c94887845d44258dc120ae1a966dfbbb7b09331e
from fabric.network import disconnect_all
from fabric.tasks import execute
import string
import pdb
from StringIO import StringIO
import re
import fabric_helper
<<<<<<< HEAD
=======
import pdb
>>>>>>> c94887845d44258dc120ae1a966dfbbb7b09331e
import logging
import random
import time

<<<<<<< HEAD
#Need to get this from config.py file
env.user='root'
env.password='passwd'
=======
env.user='hlim'
env.password='Testlab'
>>>>>>> c94887845d44258dc120ae1a966dfbbb7b09331e

random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

class am_service(object):
	
	def __init__(self, am_ip):
		env.host_string = am_ip
<<<<<<< HEAD
		self.fh = fabric_helper.FdsFabricHelper('AMAgent', am_ip)
		self.fds_log = '/tmp'
		self.fds_bin = '/fds/bin'
		#self.fdsconsole= '/fds/sbin/fdsconsole.py'
		#self.fds_sbin = '/fds/sbin'
		self.fdsconsole= '{}/fdsconsole.py'.format(self.fds_sbin)
		self.fds_sbin= '/home/hlim/projects/fds-src/source/tools'
=======
		self.fh = fabric_helper.FabricHelper('AMAgent', am_ip)
		self.fds_log = '/tmp'
		#self.fds_sbin = '/fds/sbin'
		self.fds_sbin = '/home/hlim/projects/fds-src/source/tools'

		self.am_pid = self.fh.get_service_pid()
		self.am_uuid = self.fh.get_service_uuid(self.am_pid)
>>>>>>> c94887845d44258dc120ae1a966dfbbb7b09331e
		self.am_port = 7000
		self.fds_dir = '/fds'

	def start(self):
<<<<<<< HEAD
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
=======
		#run('ifconfig eth0')
		with cd('{}'.format(self.fds_sbin)):
			#sudo('./fdsconsole.py service removeService {} am > {}/cli.out 2>&1'.format(self.am_uuid, self.fds_log))
			sudo('bash -c \"(nohup ./AMAgent --fds-root=%s 0<&- &> %s/am.out &) \"'.format(self.fds_dir, self.fds_log))

	def stop(self):
		#run('pkill -9 bare_am')
		output = run('ps auxw | grep -i amagent')
		if re.search('170483924568920640', output):
			print 'true'
		#run('ls -ltr')
			
	def kill(self):

		if self.am_pid > 0:
			log.info('Killing AMAgent service')
			cmd_status= sudo('kill -9 {}'.format(self.am_pid))

			if cmd_status.return_code == 0:
				log.info('Killed AMAgent service')
>>>>>>> c94887845d44258dc120ae1a966dfbbb7b09331e

			else:
				log.warn('Failed to kill AMAgent service {}'.format(cmd_status))
			
		else:
			log.info('AMAgent service is not running.')

	def remove(self):
<<<<<<< HEAD
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
		

=======

		
		with cd('{}'.format(self.fds_sbin)):
			#run('./fdsconsole.py service removeService {} am > {}/cli.out 2>&1'.format(self.am_uuid, self.fds_log))
			sudo('./fdsconsole.py service removeService {} am > cli.out 2>&1'.format(self.am_uuid))
		
	def add(self):
		with cd('{}'.format(self.fds_sbin)):
			#run('./fdsconsole.py service addService {} am > {}/cli.out 2>&1'.format(self.am_uuid, self.fds_log))
			run('./fdsconsole.py service addService {} am > cli.out 2>&1'.format(self.am_uuid))

		

#instance = am_service()
#am_util.add_class_methods_as_module_level_functions_for_fabric(instance, __name__)
#instance.special()
>>>>>>> c94887845d44258dc120ae1a966dfbbb7b09331e

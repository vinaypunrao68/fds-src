from __future__ import with_statement
from fabric import tasks
from fabric.api import *
from fabric.contrib.console import confirm
from fabric.network import disconnect_all
from fabric.tasks import execute
import string
import pdb
from StringIO import StringIO
import re
import fabric_helper
import pdb
import logging
import random
import time

env.user='hlim'
env.password='Testlab'

random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

class am_service(object):
	
	def __init__(self, am_ip):
		env.host_string = am_ip
		self.fh = fabric_helper.FabricHelper('AMAgent', am_ip)
		self.fds_log = '/tmp'
		#self.fds_bin = '/fds/bin'
		self.fds_bin = '/home/hlim/projects/fds-src/source/tools'

		self.am_pid = self.fh.get_service_pid()
		self.platform_uuid = self.fh.get_platform_uuid(self.am_pid)
		self.am_port = 7000
		self.fds_dir = '/fds'

	def start(self):
		#run('ifconfig eth0')
		with cd('{}'.format(self.fds_bin)):
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

			else:
				log.warn('Failed to kill AMAgent service {}'.format(cmd_status))
			
		else:
			log.info('AMAgent service is not running.')

	def remove(self, node_ip):

		node_uuid = self.fh.get_node_uuid(node_ip)
		pdb.set_trace()
		with cd('{}'.format(self.fds_bin)):
			#run('./fdsconsole.py service removeService {} am > {}/cli.out 2>&1'.format(self.am_uuid, self.fds_log))
			sudo('./fdsconsole.py service removeService {} am > cli.out 2>&1'.format(node_uuid))
		
	def add(self):
		with cd('{}'.format(self.fds_bin)):
			#run('./fdsconsole.py service addService {} am > {}/cli.out 2>&1'.format(self.am_uuid, self.fds_log))
			run('./fdsconsole.py service addService {} am > cli.out 2>&1'.format(self.node_uuid))

		

#instance = am_service()
#am_util.add_class_methods_as_module_level_functions_for_fabric(instance, __name__)
#instance.special()

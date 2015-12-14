from fabric.api import *
from fabric.contrib.files import *
import random
import time
import logging
import shlex
import os
import pdb
from StringIO import StringIO
import sys

sys.path.insert(0, '../scale-framework')
import config

random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)


class FabricHelper():
    def __init__(self, node_ip):
	env.user=config.SSH_USER
	env.password=config.SSH_PASSWORD
	env.host_string=node_ip


    def get_service_pid(self, node_service):
	'''
	This function takes ip address and FDS service and returns its PID

	ATTRIBUTES:
	----------
	service -       FDS service (e.g AMAgent, DataMgr, etc)
	ip_address -    node IP address

	Returns:
	--------
	Returns the PID of the service
	'''

        #with settings(hide('running','commands', 'stdout', 'stderr')):
        with settings(warn_only=True and hide('running','commands', 'stdout', 'stderr')):
            om_url = 'com.formationds.om.Main'
            #if om_url.find(node_service) or node_service == om_url:
            if node_service == om_url:
                #service_pid = run("ps auxw | grep {} | grep -v grep | awk '{print $2}'".format(node_service))
                service_pid = run("ps auxw | grep 'com.formationds.om.Main' | grep -v grep | awk '{print $2}'")

            else:
                service_pid = run('pgrep {}'.format(node_service))

            if service_pid.return_code == 0:
                return service_pid

            else:
                log.warning("Unable to locate {} service PID".format(node_service))
                return None


    def get_platform_uuid(self, service_pid):
	'''
    DEPRECATED - DO NOT USE
	This function takes FDS service pid and returns its platform uuid

	ATTRIBUTES:
	----------
	service_pid - FDS service pid 

	Returns:
	--------
	Returns platform uuid 
	'''

	if service_pid > 0:
		with settings(warn_only=True and hide('running','commands', 'stdout', 'stderr')):

			service_uuid = run('ps -www --no-headers {}'.format(service_pid))
			if service_uuid.return_code == 0:
				uuid = shlex.split(service_uuid)
				svc_uuid = uuid[8].split('=')
				return svc_uuid[1]

			else:
				log.warning("Unable to locate {} platform service uuid".format(self.node_service))

	else:
		log.warning("{} service is not running".format(self.node_service))

    def get_node_uuid(self, node_ip):
	'''
    DEPRECATED - DO NOT USE
	This function queries and returns node_uuid

	ATTRIBUTES:
	----------
	Takes node_ip address

	Returns:
	--------
	Returns node uuid
	'''

	if exists('{}'.format(self.fdsconsole)):
		with settings(warn_only=True and hide('running','commands', 'stdout', 'stderr')):
			with cd('{}'.format(self.fds_sbin)):

				#run('./fdsconsole.py domain listServices local', stdout=self.sio)
				cmd_output = sudo('./fdsconsole.py domain listServices local')
				n_uuid = cmd_output.split()

				for i in range(len(n_uuid)):
					if n_uuid[i] == '127.0.0.1' or n_uuid[i] == node_ip:
						if n_uuid[i+2] == 'pm':
							node_uuid = n_uuid[i-2]
							#convert hex to decimal
							return int(node_uuid, 16)

	else:
		log.warning("Unable to locate {}".format(self.fdsconsole))
                cmd_output = run('./fdsconsole.py domain listServices local')
                n_uuid = cmd_output.split()

                for i in range(len(n_uuid)):
					print n_uuid[i]
					if n_uuid[i] == node_ip:
						if n_uuid[i+2] == 'pm':
							node_uuid = n_uuid[i-2]
							return node_uuid



from fabric.api import *
import random
import time
import logging
import shlex

random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

class FabricHelper():
    def __init__(self, fds_service, fds_node):
	env.user='hlim'
	env.password='Testlab'
	env.host_string=fds_node
	self.node_service=fds_service

    def get_service_pid(self):
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
	#env.user = 'hlim' 
	#env.password = 'Testlab' 
	#env.host_string = node_ip_address

	#with settings(hide('running','commands', 'stdout', 'stderr')):
	with settings(warn_only=True and hide('running','commands', 'stdout', 'stderr')):

		service_pid = run('pgrep {}'.format(self.node_service))
		if service_pid.return_code == 0:
			return service_pid

		else:
			log.warning("Unable to locate {} service PID".format(self.node_service))


    def get_service_uuid(self, service_pid):
	'''
	This function takes FDS service pid and returns its FDS uuid

	ATTRIBUTES:
	----------
	service_pid - FDS service pid 

	Returns:
	--------
	Returns FDS uuid 
	'''

	if service_pid > 0:
		with settings(warn_only=True and hide('running','commands', 'stdout', 'stderr')):

			service_uuid = run('ps -www --no-headers {}'.format(service_pid))
			if service_uuid.return_code == 0:
				uuid = shlex.split(service_uuid)
				svc_uuid = uuid[8].split('=')
				return svc_uuid[1]

			else:
				log.warning("Unable to locate {} service uuid".format(self.node_service))

	else:
		log.warning("{} service is not running".format(self.node_service))

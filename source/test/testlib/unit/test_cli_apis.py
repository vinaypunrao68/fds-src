import logging
import unittest
import os
import sys

from fds.services.node_service import NodeService
from fds.services.users_service import UsersService
from fds.services.volume_service import VolumeService
from fds.services.users_service import UsersService
from fds.services.local_domain_service import LocalDomainService
from fds.services.snapshot_policy_service import SnapshotPolicyService
from fds.services.fds_auth import FdsAuth

from fds.model.node_state import NodeState
from fds.model.volume import Volume
from fds.model.domain import Domain

import pdb
import sys

sys.path.append("..")
import am

class TestRESTAPI(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)

    fdsauth = FdsAuth()
    fdsauth.login()

    def test_get_hostname(self):
    	self.log.info(TestRESTAPI.test_get_hostname.__name__)
    	self.log.info('get_hostname= {}'.format(self.fdsauth.get_hostname()))

    def test_get_domains(self):
    	self.log.info(TestRESTAPI.test_get_domains.__name__)
    	lds = LocalDomainService(self.fdsauth)
    	domains_list = lds.get_local_domains()

    	for domain in domains_list:
    		self.log.info('domain.name= {}'.format(domain.name))
    		self.log.info('domain.id= {}'.format(domain.id))
    		self.log.info('domain.site= {}'.format(domain.site))
    		print '==============='

    def test_get_volume_list(self):
    	self.log.info(TestRESTAPI.test_get_volume_list.__name__)
    	volServ = VolumeService(self.fdsauth)
    	volumes_list = volServ.list_volumes()

    	for volume in volumes_list:
            self.log.info('Volume.name= {}'.format(volume.name))
            self.log.info('Volume.id= {}'.format(volume.id))
            self.log.info('Volume.priority= {}'.format(volume.priority))
            self.log.info('Volume.type= {}'.format(volume.type))
            self.log.info('Volume.state= {}'.format(volume.state))
            self.log.info('Volume.current_size= {}'.format(volume.current_size))
            self.log.info('Volume.current_unit= {}'.format(volume.current_units))            
            self.log.info('===============')

    def test_create_volume(self):
        vservice = VolumeService(self.fdsauth)
        volume = Volume()
        new_volume = "testvolumecreation3"
        new_type = "block"
        volume.name = "{}".format(new_volume)
        volume.type = "{}".format(new_type)
        vservice.create_volume(volume)
        
        volCheck = VolumeService(self.fdsauth)
        volumes_list = volCheck.list_volumes()
        for volume in volumes_list:
            if volume.name == new_volume:
                self.log.info("Volume {} has been created.".format(volume.name))

    def test_get_node_list(self):
    	self.log.info(TestRESTAPI.test_get_node_list.__name__)
    	nodeService = NodeService(self.fdsauth)
    	nodes_list = nodeService.list_nodes()
    	
    	for node in nodes_list:
            self.log.info('node = {}'.format(node))
            self.log.info('node.name = {}'.format(node.name))
            self.log.info('node.ip_v4_address = {}'.format(node.ip_v4_address))
            self.log.info('node.id= {}'.format(node.id))
            self.log.info('node.state= {}'.format(node.state))
            self.log.info('node.service= {}'.format(node.services))
            self.log.info('===============')    


    def test_deactivate_node(self):
    	self.log.info(TestRESTAPI.test_deactivate_node.__name__)
    	nservice = NodeService(self.fdsauth)
    	nodeS2 = NodeState()
    	
    	nodes_list = nservice.list_nodes()
    
    	for node in nodes_list:
    		nodeS2.am=False
    		nservice.deactivate_node(node.id, nodeS2)
    		print '==============='
    
    	#check node status after deactivating
    	for node in nodes_list:

            self.log.info('node.ip_v4_address = {}'.format(node.ip_v4_address))
            self.log.info(node.services["AM"][0].auto_name + " " + node.services["AM"][0].status)
            self.log.info(node.services["SM"][0].auto_name + " " + node.services["SM"][0].status)
            self.log.info(node.services["DM"][0].auto_name + " " + node.services["DM"][0].status)
            self.log.info(node.services["PM"][0].auto_name + " " + node.services["PM"][0].status)
            self.log.info('===============')
            
           
    		#self.log.info('node.state.am= {}'.format(node.am))
    		#self.log.info('node.state.sm= {}'.format(node.sm))
    		#self.log.info('node.state.dm= {}'.format(node.dm))

if __name__ == '__main__':
	    unittest.main()


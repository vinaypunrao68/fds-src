import logging
import unittest
import os
import sys

from fdscli.services.node_service import NodeService
from fdscli.services.users_service import UsersService
from fdscli.services.volume_service import VolumeService
from fdscli.services.users_service import UsersService
from fdscli.services.local_domain_service import LocalDomainService
from fdscli.services.snapshot_policy_service import SnapshotPolicyService
from fdscli.services.fds_auth import FdsAuth

from fdscli.model.platform.node import Node
from fdscli.model.volume.volume import Volume
from fdscli.model.platform.domain import Domain

import pdb
import sys

sys.path.append("..")
#import AM

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
            #self.log.info('Volume.priority= {}'.format(volume.priority))
            #self.log.info('Volume.type= {}'.format(volume.type))
            #self.log.info('Volume.state= {}'.format(volume.state))
            #self.log.info('Volume.current_size= {}'.format(volume.current_size))
            #self.log.info('Volume.current_unit= {}'.format(volume.current_units))            
            self.log.info('===============')

    def test_create_volume(self):
        vservice = VolumeService(self.fdsauth)
        volume = Volume()
        new_volume = "create_volume_test"
        new_type = "object"
        new_max_object_size="1"
        new_max_object_size_unit="MB"
        volume.name = "{}".format(new_volume)
        volume.type = "{}".format(new_type)
        volume.max_object_size="{}".format(new_max_object_size)
        volume.max_object_size_unit="{}".format(new_max_object_size_unit)
        vservice.create_volume(volume)

        
        volCheck = VolumeService(self.fdsauth)
        volumes_list = volCheck.list_volumes()
        for volume in volumes_list:
            if volume.name == new_volume:
                self.log.info("Volume {} has been created.".format(volume.name))

        #find volume by name
        volname = vservice.find_volume_by_name(new_volume)
        print "find_volume_by_name = %s" %volname.name

    def _test_delete_volume(self):
        new_volume = "create_volume_test"
        volCheck = VolumeService(self.fdsauth)
        volumes_list = volCheck.list_volumes()
        for volume in volumes_list:
            if volume.name == new_volume:
                volID = volume.id

        #find volume by name
        volname = volCheck.delete_volume(volID)
        volumes_list = volCheck.list_volumes()
        for volume in volumes_list:
            if volume.name == new_volume:
                print "Failed to delete volume %s" %volume.name

    def test_create_snapshot(self):
        new_volume = "create_volume_test"
        volCheck = VolumeService(self.fdsauth)
        volumes_list = volCheck.list_volumes()
        for volume in volumes_list:
            if volume.name == new_volume:
                pdb.set_trace()
                volID = volume.id
                volName = volume.name

        #find volume by name
        volCheck.create_snapshot("%s_snapshot" %volName)
        pdb.set_trace()

    def _test_get_node_list(self):
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


    def _test_stop_am(self):
    	self.log.info(TestRESTAPI.test_stop_am.__name__)
    	nservice = NodeService(self.fdsauth)
    	nodeS2 = Node()
    	
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

#    def test_start_am(self):
#        self.log.info(TestRESTAPI.test_start_am.__name__)
#        amobj = AM.am_service()
#        amobj.start('10.3.79.115')	
#
#    def _test_stop_am(self):
#        self.log.info(TestRESTAPI.test_stop_am.__name__)
#        amobj = AM.am_service()
#        amobj.stop('10.3.79.115')	
#
#    def _test_add_am_service(self):
#        self.log.info(TestRESTAPI.test_add_am_service.__name__)
#        amobj = AM.am_service()
#        amobj.add('10.3.79.115')	
#
#    def _test_remove_am_service(self):
#        self.log.info(TestRESTAPI.test_remove_am_service.__name__)
#        amobj = AM.am_service()
#        amobj.remove('10.3.79.115')	
#################

if __name__ == '__main__':
	    unittest.main()


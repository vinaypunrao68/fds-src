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
import AM

class TestRESTAPI2(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)

    fdsauth = FdsAuth()
    fdsauth.login()


    def test_get_service(self):
    	self.log.info(TestRESTAPI2.test_get_service.__name__)
    	nservice = NodeService(self.fdsauth)
    	nodeS2 = NodeState()
    	
    	nodes_list = nservice.list_nodes()
    
    	#check node status after deactivating
    	for node in nodes_list:

            self.log.info('node.ip_v4_address = {}'.format(node.ip_v4_address))
            self.log.info(node.services["AM"][0].auto_name + " " + node.services["AM"][0].status + " service.id = {} ".format(node.services['AM'][0].id) + "node.id={} ".format(node.id))
            self.log.info(node.services["SM"][0].auto_name + " " + node.services["SM"][0].status + " service.id = {} ".format(node.services['SM'][0].id) + "node.id={} ".format(node.id))
            self.log.info(node.services["DM"][0].auto_name + " " + node.services["DM"][0].status + " service.id = {} ".format(node.services['DM'][0].id) + "node.id={} ".format(node.id))
            self.log.info(node.services["PM"][0].auto_name + " " + node.services["PM"][0].status + " service.id = {} ".format(node.services['PM'][0].id) + "node.id={} ".format(node.id))
            self.log.info('===============')
            
           
    		#self.log.info('node.state.am= {}'.format(node.am))
    		#self.log.info('node.state.sm= {}'.format(node.sm))
    		#self.log.info('node.state.dm= {}'.format(node.dm))


#################

if __name__ == '__main__':
	    unittest.main()


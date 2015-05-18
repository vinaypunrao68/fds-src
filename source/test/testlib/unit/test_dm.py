import logging
import unittest
import pdb
import sys
sys.path.append("..")
import DM 

class TestDM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    dm_obj = DM.dm_service()
    node_ip = '10.3.79.115'

    def _test_start(self):
	self.log.info(TestDM.test_start.__name__)
        self.assertTrue(self.dm_obj.start('{}'.format(self.node_ip)), True)

    def _test_stop(self):
	self.log.info(TestDM.test_stop.__name__)
        self.assertTrue(self.dm_obj.stop('{}'.format(self.node_ip)), True)
                
    def test_kill(self):
	pdb.set_trace()
	self.log.info(TestDM.test_kill.__name__)
        self.assertTrue(self.dm_obj.kill('{}'.format(self.node_ip)), True)


if __name__ == '__main__':
    unittest.main()


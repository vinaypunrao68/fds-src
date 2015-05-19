import logging
import unittest
import pdb
import sys
sys.path.append("..")
import SM
import config

class TestSM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    sm_obj = SM.sm_service()
    node_ip = '10.3.100.9'

    def test_start(self):
        self.log.info(TestSM.test_start.__name__)
        self.assertTrue(self.sm_obj.start('{}'.format(self.node_ip)), True)

    def test_stop(self):
        self.log.info(TestSM.test_stop.__name__)
        self.assertTrue(self.sm_obj.stop('{}'.format(self.node_ip)), True)
                
    def test_kill(self):
        self.log.info(TestSM.test_kill.__name__)
        self.assertTrue(self.sm_obj.kill('{}'.format(self.node_ip)), True)

    def test_add(self):
        self.log.info(TestSM.test_add.__name__)
        self.assertTrue(self.sm_obj.add('{}'.format(self.node_ip)), True)

    def test_remove(self):
        self.log.info(TestSM.test_remove.__name__)
        #self.assertTrue(self.sm_obj.remove('{}'.format(self.node_ip)), True)
        self.assertFalse(self.sm_obj.remove('{}'.format(self.node_ip)), False)




if __name__ == '__main__':
    unittest.main()


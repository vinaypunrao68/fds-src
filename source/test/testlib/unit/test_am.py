import logging
import unittest
import pdb
import sys
sys.path.append("..")
import AM

class TestAM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    am_obj = AM.am_service()
    node_ip = '127.0.0.1'

    def test_start(self):
	self.log.info(TestAM.test_start.__name__)
        self.assertTrue(self.am_obj.start('{}'.format(self.node_ip)), True)

    def test_stop(self):
	self.log.info(TestAM.test_stop.__name__)
        self.assertTrue(self.am_obj.stop('{}'.format(self.node_ip)), True)
                
    def test_kill(self):
	self.log.info(TestAM.test_kill.__name__)
        self.assertTrue(self.am_obj.kill('{}'.format(self.node_ip)), True)


if __name__ == '__main__':
    unittest.main()


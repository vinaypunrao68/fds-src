import logging
import unittest
import pdb
import sys
sys.path.append("..")
import AM

class TestDM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    am_obj = AM.am_service()
    om_ip = '127.0.0.1'

    def test_start(self):
	self.log.info(TestDM.test_start.__name__)
        self.assertTrue(self.am_obj.start('{}'.format(self.om_ip)), True)

    def test_stop(self):
	self.log.info(TestDM.test_stop.__name__)
        self.assertTrue(self.am_obj.stop('{}'.format(self.om_ip)), True)
                
    def test_kill(self):
	self.log.info(TestDM.test_kill.__name__)
        self.assertTrue(self.am_obj.kill('{}'.format(self.om_ip)), True)


if __name__ == '__main__':
    unittest.main()


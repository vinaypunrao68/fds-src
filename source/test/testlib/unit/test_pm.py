import logging
import unittest
import pdb
import sys
sys.path.append("..")
import PM 

class TestPM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    pm_obj = PM.pm_service()
    node_ip = '10.3.100.7'

    def test_start(self):
	pdb.set_trace()
	self.log.info(TestPM.test_start.__name__)
        self.assertTrue(self.pm_obj.start('{}'.format(self.node_ip)), True)

    def test_stop(self):
	pdb.set_trace()
	self.log.info(TestPM.test_stop.__name__)
        self.assertTrue(self.pm_obj.stop('{}'.format(self.node_ip)), True)
                

if __name__ == '__main__':
    unittest.main()


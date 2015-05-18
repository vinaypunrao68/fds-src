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
    om_node = '127.0.0.1'

    def _test_start(self):
	self.log.info(TestDM.test_start.__name__)
        self.assertTrue(self.dm_obj.start('{}'.format(self.om_node)), True)

    def _test_stop(self):
	self.log.info(TestDM.test_stop.__name__)
        self.assertTrue(self.dm_obj.stop('{}'.format(self.om_node)), True)
                
    def test_kill(self):
	self.log.info(TestDM.test_kill.__name__)
        self.assertTrue(self.dm_obj.kill('{}'.format(self.om_node)), True)


if __name__ == '__main__':
    unittest.main()


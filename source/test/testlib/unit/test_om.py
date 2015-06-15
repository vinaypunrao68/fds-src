import logging
import unittest
import pdb
import sys
sys.path.append("..")
import OM
import fabric_helper as fh

class TestOM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    om_obj = OM.OMService()
    node_ip = '10.3.19.0'
    fh_obj = fh.FabricHelper(node_ip)

    def test_start(self):
        self.log.info(TestOM.test_start.__name__)
        self.assertTrue(self.om_obj.start('{}'.format(self.node_ip)), True)

    def test_stop(self):
        self.log.info(TestOM.test_stop.__name__)
        self.assertTrue(self.om_obj.stop('{}'.format(self.node_ip)), True)
                
    def test_kill(self):
        self.log.info(TestOM.test_kill.__name__)
        self.assertTrue(self.om_obj.kill('{}'.format(self.node_ip)), True)


if __name__ == '__main__':
    unittest.main()


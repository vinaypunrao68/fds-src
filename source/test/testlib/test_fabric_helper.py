import logging
import unittest
import pdb
import fabric_helper as fh

class TestFabricHelper(unittest.TestCase):

    logging.basicConfig(level=logging.INFO,
			format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    node_ip='127.0.0.1'
    fhObj = fh.FabricHelper(node_ip)

    def test_get_service_pid(self):
        srv_id = self.fhObj.get_service_pid('om.Main')
        print srv_id

if __name__ == '__main__':
    unittest.main()


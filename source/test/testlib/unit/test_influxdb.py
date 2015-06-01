import logging
import unittest
import pdb
import sys
sys.path.append("..")
import INFLUXDB
import fabric_helper as fh

class TestINFLUXDB(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    influxdb_obj = INFLUXDB.InfluxdbService()
    node_ip = '10.3.61.58'

    def _test_start(self):
        self.log.info(TestINFLUXDB.test_start.__name__)
        self.assertTrue(self.influxdb_obj.start('{}'.format(self.node_ip)), True)

    def test_stop(self):
        self.log.info(TestINFLUXDB.test_stop.__name__)
        self.assertTrue(self.influxdb_obj.stop('{}'.format(self.node_ip)), True)

if __name__ == '__main__':
    unittest.main()


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
    dm_obj = DM.DMService()
    node_ip = '10.3.87.193'
    nodeList = ['10.3.87.194']

    def _test_start(self):
	for eachNode in self.nodeList:
		self.log.info(TestDM.test_start.__name__)
		self.assertTrue(self.dm_obj.start('{}'.format(eachNode)), True)

    def _test_stop(self):
	for eachNode in self.nodeList:
		self.log.info(TestDM.test_stop.__name__)
		self.assertTrue(self.dm_obj.stop('{}'.format(eachNode)), True)
                
    def _test_kill(self):
	for eachNode in self.nodeList:
		self.log.info(TestDM.test_kill.__name__)
		self.assertTrue(self.dm_obj.kill('{}'.format(eachNode)), True)

    def test_add(self):
	for eachNode in self.nodeList:
		self.log.info(TestDM.test_add.__name__)
		self.assertTrue(self.dm_obj.add('{}'.format(eachNode)), True)

    def test_remove(self):
	for eachNode in self.nodeList:
		self.log.info(TestDM.test_remove.__name__)
		self.assertTrue(self.dm_obj.remove('{}'.format(eachNode)), True)

if __name__ == '__main__':
    unittest.main()


import logging
import unittest
import sys
sys.path.append("..")


class TestSM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    sm_obj = sm.SMService()
    node_ip = '10.3.87.193'
    nodeList= ['10.3.87.194']

    def test_start(self):
        for eachNode in self.nodeList:
		self.log.info(TestSM.test_start.__name__)
		self.assertTrue(self.sm_obj.start('{}'.format(eachNode)), True)

    def test_stop(self):
        for eachNode in self.nodeList:
		self.log.info(TestSM.test_stop.__name__)
		self.assertTrue(self.sm_obj.stop('{}'.format(eachNode)), True)
                
    def test_kill(self):
        for eachNode in self.nodeList:
		self.log.info(TestSM.test_kill.__name__)
		self.assertTrue(self.sm_obj.kill('{}'.format(eachNode)), True)

    def test_add(self):
        for eachNode in self.nodeList:
		self.log.info(TestSM.test_add.__name__)
		self.assertTrue(self.sm_obj.add('{}'.format(eachNode)), True)

    def test_remove(self):
        for eachNode in self.nodeList:
		self.log.info(TestSM.test_remove.__name__)
		self.assertTrue(self.sm_obj.remove('{}'.format(eachNode)), True)




if __name__ == '__main__':
    unittest.main()


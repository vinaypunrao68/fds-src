import logging
import unittest
import pdb
import sys
sys.path.append("..")
import am

class TestDM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    am_obj = am.am_service()

    def test_start(self):
        self.assertTrue(self.am_obj.start('10.3.82.43'), True)

    def test_stop(self):
		#am_obj = am.am_service()
        #self.assertTrue(self.am_obj.stop('10.3.82.41'), True)
        #self.assertTrue(self.am_obj.stop('10.3.82.42'), True)
        self.assertTrue(self.am_obj.stop('10.3.82.43'), True)
                
    def test_kill(self):
        #am_obj = am.am_service()
        #self.assertTrue(self.am_obj.kill('10.3.82.42'), True)
        self.assertTrue(self.am_obj.kill('10.3.82.43'), True)


if __name__ == '__main__':
    unittest.main()


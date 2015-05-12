import logging
import unittest
import pdb
import sys
sys.path.append("..")
import dm

class TestDM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
	format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    dm_obj = dm.dm_service()

    def _test_start(self):
        self.assertTrue(self.dm_obj.start('10.3.82.43'), True)

    def _test_stop(self):
		#dm_obj = am.am_service()
        #self.assertTrue(self.dm_obj.stop('10.3.82.41'), True)
        #self.assertTrue(self.dm_obj.stop('10.3.82.42'), True)
        self.assertTrue(self.dm_obj.stop('10.3.82.43'), True)
                
    def test_kill(self):
        #dm_obj = am.am_service()
        #self.assertTrue(self.dm_obj.kill('10.3.82.42'), True)
        self.assertTrue(self.dm_obj.kill('10.3.82.43'), True)


if __name__ == '__main__':
    unittest.main()


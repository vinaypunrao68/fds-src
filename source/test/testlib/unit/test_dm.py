import logging
import unittest
import pdb
import sys
sys.path.append("..")
import dm

class TestAM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
			format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    dm_obj = dm.dm_service()

    def _test_start(self):
	self.dm_obj.start('10.3.16.213')

    def _test_stop(self):
		#dm_obj = am.am_service()
        self.dm_obj.stop('10.3.16.213')
        pdb.set_trace()
        self.dm_obj.stop('10.3.16.214')

    def test_kill(self):
	#dm_obj = am.am_service()
	self.dm_obj.kill('10.3.82.42')
	self.dm_obj.kill('10.3.82.41')


if __name__ == '__main__':
    unittest.main()


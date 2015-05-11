import logging
import unittest
import pdb
import sys
sys.path.append("..")
import am

class TestAM(unittest.TestCase):
    logging.basicConfig(level=logging.INFO,
			format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    am_obj = am.am_service()

    def test_start(self):
		self.am_obj.start('10.3.16.213')

    def test_stop(self):
		#am_obj = am.am_service()
        self.am_obj.stop('10.3.16.213')
        pdb.set_trace()
        self.am_obj.stop('10.3.16.214')

    def test_kill(self):
		#am_obj = am.am_service()
		self.am_obj.kill('10.3.16.213')


if __name__ == '__main__':
    unittest.main()


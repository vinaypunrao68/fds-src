import am
import logging
import unittest
import pdb

class TestAM(unittest.TestCase):

	logging.basicConfig(level=logging.INFO,
			format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
			datefmt='%m-%d %H:%M')
	log = logging.getLogger(__name__)

	def _test_start(self):
		am_obj = am.am_service('10.3.64.96')
		am_obj.start()

	def _test_add(self):
		#am_obj = am.am_service('10.3.64.96')
		am_obj = am.am_service('127.0.0.1')
		am_obj.add()

	def test_kill(self):
		#am_obj = am.am_service('10.3.64.96')
		am_obj = am.am_service('127.0.0.1')
		am_obj.kill()

	def _test_stop(self):
		#am_obj = am.am_service('10.3.64.96')
		am_obj = am.am_service('10.2.30.137')
		am_obj.stop()

	def _test_all_service(self):
		pdb.set_trace()
		am_obj = am.am_service('10.3.64.96')
		am_obj.all_service()
	

if __name__ == '__main__':
    unittest.main()


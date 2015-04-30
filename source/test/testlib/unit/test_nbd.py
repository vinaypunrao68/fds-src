##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: nbd.py                                                               #
##############################################################################
import unittest
import logging
import os
import sys

nbd_path = os.path.abspath(os.path.join('..', 'utils'))
sys.path.append(nbd_path)

import utils.nbd as nbd

class TestNbd(unittest.TestCase):
    
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    
    def setUp(self):
        self.nbd_obj = nbd.NBD(volume="fdstest", mount_point="/fdsmount",
                               ipaddress="10.2.10.200")
    
    def test_test(self):
        self.nbd_obj.check_status()
        self.assertTrue(True)
        self.nbd_obj.attach_nbd()
    

if __name__ == '__main__':
    unittest.main()
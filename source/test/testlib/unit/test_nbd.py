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
        self.nbd_obj = nbd.NBD(hostname="10.2.10.200", mount_point="/fdsmount")

    def test_attach_volume(self):
        try:
            volume = "fdstest"
            self.nbd_obj.check_status()
            (device, code) = self.nbd_obj.attach(volume)
            self.assertEqual(code, 0)
            self.assertEqual(self.nbd_obj.detach(volume), 0)
        except:
            raise AssertionError('shouldn\'t raise an exception')

    def test_format_disk(self):
        volume = "fdstest"
        (device, code) = self.nbd_obj.attach(volume)
        self.assertEqual(code, 0)
        status = self.nbd_obj.format_disk(device)
        self.assertTrue(status)
        # mount the device
        code = self.nbd_obj.mount("/fdsmount", device)
        self.assertEqual(code, 0)


if __name__ == '__main__':
    unittest.main()

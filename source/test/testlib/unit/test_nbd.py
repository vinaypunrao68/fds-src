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

import nbd as nbd

class TestNbd(unittest.TestCase):

    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)

    def setUp(self):
        self.nbd_obj = nbd.NBD(hostname="10.2.10.200")

    def test_format_disk(self):
        volume = "fdsnbdtest"
        mount_point = "/fdsmount"
        device = self.nbd_obj.attach(volume)
        self.nbd_obj.format_disk(device)
        # mount the device
        self.nbd_obj.mount(mount_point, device)
        self.nbd_obj.detach(volume)


if __name__ == '__main__':
    unittest.main()

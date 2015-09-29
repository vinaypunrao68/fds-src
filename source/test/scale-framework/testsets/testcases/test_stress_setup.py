#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.

import ConfigParser

import s3
import lib
import config
import unittest
import json
import testsets.testcase as testcase
import block_volumes
import s3_volumes
import logging

class TestStressSetup(testcase.FDSTestCase):

    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)

    '''
    Setup volumes for the stress test
    '''

    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        print(om_ip_address)
        super(TestStressSetup, self).__init__(parameters=parameters,
                config_file=config_file, om_ip_address=om_ip_address)

        self.s3_vol_count = 500
        self.block_vol_count = 500
        if(config_file != None):
            self.log.info("config_file = %s", config_file)
            config = ConfigParser.ConfigParser()
            config.readfp(open(config_file))
            # TODO: Only get this if these values are set
            self.s3_vol_count = config.getint("stress", "s3_vols")
            self.block_vol_count = config.getint("stress", "block_vols")

        self.log.info("s3_vol_count = %s", self.s3_vol_count)

    '''
    Do it.
    '''
    def runTest(self):
        test_passed = False
        s3_vols = s3_volumes.S3Volumes(self.om_ip_address)
        block_vols = block_volumes.BlockVolumes(self.om_ip_address)

        try:

            #Setup
            s3_bucket_list = s3_vols.create_volumes(self.s3_vol_count,
                    "stress_test_s3")
            block_vol_list = block_vols.create_volumes(self.block_vol_count,
                    "stress_test_block", "1", "GB")

            self.log.info(s3_bucket_list)
            self.log.info(block_vol_list)

            #Yay?
            test_passed = True


        except Exception, e:
            self.log.exception(e)
            test_passed = False
        finally:
            super(self.__class__, self).reportTestCaseResult(test_passed)

    '''
    Undo it.
    '''
#    def tearDown(self):

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()

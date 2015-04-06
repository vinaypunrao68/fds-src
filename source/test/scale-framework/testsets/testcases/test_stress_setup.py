#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.

import s3
import utils
import config
import unittest
import json
import testsets.testcase as testcase

class TestStressSetup(testcase.FDSTestCase):

    '''
    Setup volumes for the stress test
    '''

    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestCreateThreehundredObjectVolumes, self).__init__(parameters=parameters,
                config_file=config_file, om_ip_address=om_ip_address)

    '''
    Do it.
    '''
    def runTest(self):
        test_passed = False
        r = None
        port = config.FDS_REST_PORT

        try:

            #Setup

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

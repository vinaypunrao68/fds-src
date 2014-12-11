#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
import logging
import sys
import traceback
import unittest

import config
import s3
import testsets.testcases.fdslib.TestUtils as TestUtils 

class FDSTestCase(unittest.TestCase):

    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)
    s3conn = s3.S3Connection(
                        config.FDS_DEFAULT_KEY_ID,
                        config.FDS_DEFAULT_SECRET_ACCESS_KEY,
                        config.FDS_DEFAULT_HOST,
                        config.FDS_AUTH_DEFAULT_PORT,       
                      )
    param = s3.TestParameters(config.FDS_DEFAULT_BUCKET_NAME,
                              config.FDS_DEFAULT_BUCKET_PREFIX,
                              config.FDS_DEFAULT_KEY_NAME,
                              config.FDS_DEFAULT_KEYS_COUNT,
                              config.FDS_DEFAULT_FILE_PATH,
                              config.TEST_DEBUG)
        
    def __init__(self, parameters=None, test_failure=False):
        """
        When run by a qaautotest module test runner,
        this method provides the test fixture allocation.

        When run by the PyUnit test runner, this method
        sets the config file specified on the command line.

        To consolidate logic, we'll let method setUp() handle it.
        """
        super(FDSTestCase, self).__init__()
        self.name = self.__class__.__name__
        self.result = None
        self.test_passed = True
        self.test_failure = test_failure
        
        if parameters:
            self.parameters = parameters
        else:
            self.parameters = TestUtils.get_config(True, config.pyUnitConfig, 
                                                   False, False,
                                                   False, "passwd")
            self.parameters['s3'] = self.s3conn
            self.parameters['s3'].conn = self.s3conn.get_s3_connection()
        
    def tearDown(self):
        """
        When run by the PyUnit test runner,
        this method provides the test fixture release.

        In the case of qaautotest, it appears you must do
        this yourself, preferably as part of the test case
        execution - the runTest() method.
        """
        pass

    def runTest(self):
        """
        Define this one in your specific test case class to run or
        invoke the test case execution and then do any necessary
        test fixture cleanup.

        For quautotest, return "True" if the test case passed, "False" otherwise.
        For PyUnit, use a unittest.assert* method to assess test case results.
        
        Attributes:
        -----------
        None
        
        Returns:
        --------
        ???
        """
        FDSTestCase.reportTestCaseResult(self, self.test_passed)
        
        if self.parameters["pyUnit"]:
            self.assertTrue(self.test_passed)
        else:
            return test_passed

    def reportTestCaseResult(self, test_passed):
        """
        Just a quick report of results for the test case.
        
        Attributes:
        -----------
        test_passed : boolean
            a boolean flag that tell if the test passed or failed.
            
        Returns:
        --------
        None
        """
        if test_passed:
            self.log.info("Test Case %s passed." % self.__class__.__name__)
        else:
            self.log.info("Test Case %s failed." % self.__class__.__name__)

            if self.parameters["stop_on_fail"]:
                self.test_failure = True
#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
# This file is a general boilerplate for new system test suites

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase

# Module-specific requirements
import sys
import os
import math
import filecmp

# This class contains the attributes and methods to test
# the FDS Cinder interface to create a connection.
#
class TestDummyGetConn(TestCase.FDSTestCase):
    """
    This case tests that we can get a connection to Cinder
    Incomplete: True
    FeatureReady: True
    """
    def __init__(self, parameters=None):
        super(TestDummyGetConn, self).__init__(parameters)

    # This method is called by the test framework and starts all other
    # tests in the test case
    def runTest(self):
        # Currently this test is skipped
        return unittest.skip("This test is incomplete, skipped")

        # Wrap the result of the test to capture failures / exceptions
        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        super(self.__class__, self).reportTestCaseResult(test_passed)

class TestDummyVerifyDeleteVol(TestCase.FDSTestCase):
    """
    This case tests that we can delete a volume
    - Create a volume
    - Confirm volume was created successfully
    - Delete the volume
    - Confirm volume deletion
    Incomplete: True
    FeatureReady: True
    """

    def __init__(self, parameters=None):
        super(TestDummyVerifyDeleteVol, self).__init__(parameters)

    # This method is called by the test framework and starts all other
    # tests in the test case
    def runTest(self):
        # Currently this test is skipped
        return unittest.skip("This test is incomplete, skipped")

        # Wrap the result of the test to capture failures / exceptions
        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        super(self.__class__, self).reportTestCaseResult(test_passed)

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()

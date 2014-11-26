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

    def runTest(self):
        return unittest.skip("This test is incomplete, skipped")

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

    def runTest(self):
        return unittest.skip("This test is incomplete, skipped")

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()

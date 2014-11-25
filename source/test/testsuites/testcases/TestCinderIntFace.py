#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase

# Module-specific requirements
import sys
import os
import math
from filechunkio import FileChunkIO
import filecmp

# This class contains the attributes and methods to test
# the FDS Cinder interface to create a connection.
#
class TestCinderGetConn(TestCase.FDSTestCase):
    """
    This case tests that we can get a connection to Cinder
    Incomplete: True
    """
    def __init__(self, parameters=None):
        super(TestCinderGetConn, self).__init__(parameters)

    def runTest(self):
        return unittest.skip("This test is incomplete, skipped")

class TestCinderVerifyDeleteVol(TestCase.FDSTestCase):
    """
    This case tests that we can delete a volume
    Incomplete: True
    """

    def __init__(self, parameters=None):
        super(TestCinderVerifyDeleteVol, self).__init__(parameters)

    def runTest(self):
        return unittest.skip("This test is incomplete, skipped")

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()

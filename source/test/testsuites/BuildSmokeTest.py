#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSModMgt
import testcases.TestFDSSysMgt

def suiteConstruction():
    """
    Construct the ordered set of test cases that comprise the
    Build Smoke Test suite.
    """
    suite = unittest.TestSuite()

    suite.addTest(testcases.TestFDSEnvMgt.TestFDSCreateInstDir())
    suite.addTest(testcases.TestFDSModMgt.TestPMBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestOMBringUp())
    suite.addTest(testcases.TestFDSSysMgt.TestNodeActivate())

    return suite

if __name__ == '__main__':

    # Handle FDS specific commandline arguments.
    testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)

    # Get a test runner that will output an xUnit XML report for Jenkins
    runner = xmlrunner.XMLTestRunner(output='test-reports')

    test_suite = suiteConstruction()

    runner.run(test_suite)


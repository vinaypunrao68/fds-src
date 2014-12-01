#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestCinderIntFace


def suiteConstruction():
    """
    Construct the ordered set of test cases that comprise the
    Build Smoke Test suite.
    """
    suite = unittest.TestSuite()

    # Cinder tests
    suite.addTest(testcases.TestCinderIntFace.TestCinderGetConn())
    suite.addTest(testcases.TestCinderIntFace.TestCinderVerifyDeleteVol())

    return suite

if __name__ == '__main__':

    # Handle FDS specific commandline arguments.
    log_dir, failfast = testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)

    # If a test log directory was not supplied on the command line (with option "-l"),
    # then default it.
    if log_dir is None:
        log_dir = 'test-reports'

    runner = xmlrunner.XMLTestRunner(output=log_dir)

    test_suite = suiteConstruction()

    runner.run(test_suite)


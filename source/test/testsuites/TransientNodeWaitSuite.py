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

def suiteConstruction(self):
    """
    Construct the ordered set of test cases that comprise the
    test cases necessary to check whether a transient node is started.

    Note that this does not check non-transient nodes. For those, use
    suite NodeWaitSuite.
    """
    suite = unittest.TestSuite()

    # Check the node(s) according to configuration supplied with the -q cli option.
    suite.addTest(testcases.TestFDSModMgt.TestPMForTransientWait())
    suite.addTest(testcases.TestFDSModMgt.TestDMForTransientWait())
    suite.addTest(testcases.TestFDSModMgt.TestSMForTransientWait())
    suite.addTest(testcases.TestFDSModMgt.TestAMForTransientWait())

    return suite

if __name__ == '__main__':

    # Handle FDS specific commandline arguments.
    log_dir, failfast = testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)

    # If a test log directory was not supplied on the command line (with option "-l"),
    # then default it.
    if log_dir is None:
        log_dir = 'test-reports'

    # Get a test runner that will output an xUnit XML report for Jenkins
    # TODO(Greg) I've tried everything I can think of, but failfast does not
    # stop the suite upon first failure.
    runner = xmlrunner.XMLTestRunner(output=log_dir)

    test_suite = suiteConstruction(self=None)

    runner.run(test_suite)


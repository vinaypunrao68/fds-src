#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSModMgt
import testcases.TestFDSSysMgt

def suiteConstruction(self, fdsNodes=None):
    """
    Construct the ordered set of test cases that comprise the
    test cases necessary to check whether a node is shutdown.
    """
    suite = unittest.TestSuite()

    # Check the node(s) according to configuration supplied with the -q cli option
    # or if a list of nodes is provided, check them specifically.
    if fdsNodes is not None:

        for node in fdsNodes:
            suite.addTest(testcases.TestFDSModMgt.TestPMForOMVerifyShutdown(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestOMVerifyShutdown(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestPMVerifyShutdown(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestDMVerifyShutdown(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestSMVerifyShutdown(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestAMVerifyShutdown(node=node))
    else:
        suite.addTest(testcases.TestFDSModMgt.TestPMForOMVerifyShutdown())
        suite.addTest(testcases.TestFDSModMgt.TestOMVerifyShutdown())
        suite.addTest(testcases.TestFDSModMgt.TestPMVerifyShutdown())
        suite.addTest(testcases.TestFDSModMgt.TestDMVerifyShutdown())
        suite.addTest(testcases.TestFDSModMgt.TestSMVerifyShutdown())
        suite.addTest(testcases.TestFDSModMgt.TestAMVerifyShutdown())

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


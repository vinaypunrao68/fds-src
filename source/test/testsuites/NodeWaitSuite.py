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

def suiteConstruction(fdsNodes=None):
    """
    Construct the ordered set of test cases that comprise the
    test cases necessary to check whether a node is started.

    Note that this does not check non-transient nodes. For those, use
    suite TransientNodeWaitSuite.
    """
    suite = unittest.TestSuite()

    # Check the node(s) according to configuration supplied with the -q cli option
    # or if a list of nodes is provided, check them specifically.
    if fdsNodes is not None:

        for node in fdsNodes:
            suite.addTest(testcases.TestFDSModMgt.TestPMForOMWait(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestOMWait(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestPMWait(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestDMWait(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestSMWait(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestAMWait(node=node))
    else:
        suite.addTest(testcases.TestFDSModMgt.TestPMForOMWait())
        suite.addTest(testcases.TestFDSModMgt.TestOMWait())
        suite.addTest(testcases.TestFDSModMgt.TestPMWait())
        suite.addTest(testcases.TestFDSModMgt.TestDMWait())
        suite.addTest(testcases.TestFDSModMgt.TestSMWait())
        suite.addTest(testcases.TestFDSModMgt.TestAMWait())

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
    runner = xmlrunner.XMLTestRunner(output=log_dir, failfast=failfast)

    test_suite = suiteConstruction()

    runner.run(test_suite)


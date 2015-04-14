#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSServiceMgt
import testcases.TestFDSSysMgt

def suiteConstruction(self, fdsNodes=None):
    """
    Construct the ordered set of test cases that comprise the
    test cases necessary to check whether a node's Services
    are down or up accordingly due to a "shutdown action".
    """
    suite = unittest.TestSuite()

    # Check the node(s) according to configuration supplied with the -q cli option
    # or if a list of nodes is provided, check them specifically.
    #
    # With a "graceful" shutdown, we expect AM, DM, and SM to be down but OM and PM
    # up.
    if fdsNodes is not None:

        for node in fdsNodes:
            suite.addTest(testcases.TestFDSServiceMgt.TestPMForOMWait(node=node))
            suite.addTest(testcases.TestFDSServiceMgt.TestOMWait(node=node))
            suite.addTest(testcases.TestFDSServiceMgt.TestPMWait(node=node))
            suite.addTest(testcases.TestFDSServiceMgt.TestDMVerifyDown(node=node))
            suite.addTest(testcases.TestFDSServiceMgt.TestSMVerifyDown(node=node))
            suite.addTest(testcases.TestFDSServiceMgt.TestAMVerifyDown(node=node))
    else:
        suite.addTest(testcases.TestFDSServiceMgt.TestPMForOMWait())
        suite.addTest(testcases.TestFDSServiceMgt.TestOMWait())
        suite.addTest(testcases.TestFDSServiceMgt.TestPMWait())
        suite.addTest(testcases.TestFDSServiceMgt.TestDMVerifyDown())
        suite.addTest(testcases.TestFDSServiceMgt.TestSMVerifyDown())
        suite.addTest(testcases.TestFDSServiceMgt.TestAMVerifyDown())

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


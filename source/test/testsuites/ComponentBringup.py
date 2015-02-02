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
import NodeWaitSuite

def suiteConstruction(self):
    """
    Construct the ordered set of test cases that comprise the
    test cases necessary to bring upa node component by component.
    """
    suite = unittest.TestSuite()

    # Build the necessary FDS installation infrastructure assuming a development environment.
    suite.addTest(testcases.TestFDSEnvMgt.TestFDSCreateInstDir())
    suite.addTest(testcases.TestFDSEnvMgt.TestRestartRedisClean())

    # Bring up each component of the node separately, waiting until
    # one is up before proceeding to the next.
    suite.addTest(testcases.TestFDSModMgt.TestPMBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestPMWait())
    suite.addTest(testcases.TestFDSModMgt.TestOMBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestOMWait())
    suite.addTest(testcases.TestFDSModMgt.TestDMBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestDMWait())
    suite.addTest(testcases.TestFDSModMgt.TestSMBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestSMWait())
    suite.addTest(testcases.TestFDSModMgt.TestAMBringup())
    suite.addTest(testcases.TestFDSModMgt.TestAMWait())

    # Check that the node is up.
    nodeUpSuite = NodeWaitSuite.suiteConstruction(self=None)
    suite.addTest(nodeUpSuite)

    return suite

if __name__ == '__main__':

    # Handle FDS specific commandline arguments.
    log_dir, failfast = testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)

    # If a test log directory was not supplied on the command line (with option "-l"),
    # then default it.
    if log_dir is None:
        log_dir = 'test-reports'

    # Get a test runner that will output an xUnit XML report for Jenkins
    runner = xmlrunner.XMLTestRunner(output=log_dir)

    test_suite = suiteConstruction(self=None)

    runner.run(test_suite)


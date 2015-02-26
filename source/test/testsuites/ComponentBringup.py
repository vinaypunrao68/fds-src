#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSServiceMgt
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
    suite.addTest(testcases.TestFDSServiceMgt.TestPMBringUp())
    suite.addTest(testcases.TestFDSServiceMgt.TestPMWait())
    suite.addTest(testcases.TestFDSServiceMgt.TestOMBringUp())
    suite.addTest(testcases.TestFDSServiceMgt.TestOMWait())
    suite.addTest(testcases.TestFDSServiceMgt.TestDMBringUp())
    suite.addTest(testcases.TestFDSServiceMgt.TestDMWait())
    suite.addTest(testcases.TestFDSServiceMgt.TestSMBringUp())
    suite.addTest(testcases.TestFDSServiceMgt.TestSMWait())
    suite.addTest(testcases.TestFDSServiceMgt.TestAMBringup())
    suite.addTest(testcases.TestFDSServiceMgt.TestAMWait())

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


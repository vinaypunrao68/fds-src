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
import testcases.TestFDSSysLoad
import NodeWaitSuite
import BotoBLOBSuite
import NodeResilienceSuite


def suiteConstruction():
    """
    Construct the ordered set of test cases that comprise the
    Build Smoke Test suite.
    """
    suite = unittest.TestSuite()

    # Build the necessary FDS infrastructure.
    suite.addTest(testcases.TestFDSEnvMgt.TestFDSCreateInstDir())
    suite.addTest(testcases.TestFDSEnvMgt.TestRestartRedisClean())

    # Start the node(s) according to configuration supplied with the -q cli option.
    suite.addTest(testcases.TestFDSModMgt.TestPMBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestPMWait())
    suite.addTest(testcases.TestFDSModMgt.TestOMBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestOMWait())
    suite.addTest(testcases.TestFDSSysMgt.TestClusterActivate())

    # Check that all nodes are up.
    nodeUpSuite = NodeWaitSuite.suiteConstruction()
    suite.addTest(nodeUpSuite)

    # Given the nodes some time to initialize.
    suite.addTest(testcases.TestFDSSysMgt.TestNodeWait())

    # Load test.
    suite.addTest(testcases.TestFDSSysLoad.TestSmokeLoad())

    # Small/Large BLOB test using Boto.
    blobSuite = BotoBLOBSuite.suiteConstruction()
    suite.addTest(blobSuite)

    # Node Resiliency suite.
    nodeResilienceSuite = NodeResilienceSuite.suiteConstruction()
    suite.addTest(nodeResilienceSuite)

    suite.addTest(testcases.TestFDSSysMgt.TestNodeShutdown())

    # Cleanup FDS installation directory.
    suite.addTest(testcases.TestFDSEnvMgt.TestFDSDeleteInstDir())

    return suite

if __name__ == '__main__':
	
    # Handle FDS specific commandline arguments.
    print sys.argv
    log_dir, failfast = testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)

    # If a test log directory was not supplied on the command line (with option "-l"),
    # then default it.
    if log_dir is None:
        log_dir = 'test-reports'

    # Get a test runner that will output an xUnit XML report for Jenkins
    # TODO(Greg) I've tried everything I can think of, but stop-on-fail/failfast does not
    # stop the suite upon first test case failure. So I've implemented our own
    # "failfast" via the TestCase.pyUnitTCFailure global. (And besides, some system
    # test users - most notably, Jenkins - were reporting that XMLTestRunner did not
    # recognize the failfast argument.)
    #runner = xmlrunner.XMLTestRunner(output=log_dir, failfast=failfast)
    runner = xmlrunner.XMLTestRunner(output=log_dir)

    test_suite = suiteConstruction()
    print test_suite
    runner.run(test_suite)


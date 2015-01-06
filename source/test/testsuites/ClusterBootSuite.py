#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
# From the .../source/test/testsuite directory run
# ./ClusterBootSuite.py -q ./<QAAutotestConfig.ini> -d <sudo_pwd> --verbose
#
# where
#  - <QAAutotestConfig.ini> - One of TwoNodeCluster.ini or FourNodeCluster.ini found in the
#  testsuite directory, or one of your own.
#  - <sudo_pwd> - The password you use for executing sudo on your machine. Even
#  if your machine does not require a password for sudo, you must provide something, e.g. "dummy".
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
    Two Node Cluster Setup test suite.
    """
    suite = unittest.TestSuite()

    # Build the necessary FDS infrastructure.
    suite.addTest(testcases.TestFDSEnvMgt.TestFDSCreateInstDir())
    suite.addTest(testcases.TestFDSEnvMgt.TestRestartRedisClean())

    # Start the the OM's PM.
    suite.addTest(testcases.TestFDSModMgt.TestPMForOMBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestPMForOMWait())

    # Now start OM.
    suite.addTest(testcases.TestFDSModMgt.TestOMBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestOMWait())

    # Start the remaining PMs
    suite.addTest(testcases.TestFDSModMgt.TestPMBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestPMWait())

    # Activate the cluster.
    suite.addTest(testcases.TestFDSSysMgt.TestClusterActivate())

    # Bring up any AMs.
    suite.addTest(testcases.TestFDSModMgt.TestAMBringup())

    # Check that all nodes are up.
    nodeUpSuite = NodeWaitSuite.suiteConstruction()
    suite.addTest(nodeUpSuite)

    # Given the nodes some time to initialize.
    suite.addTest(testcases.TestFDSSysMgt.TestNodeWait())

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


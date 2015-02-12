#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
#
# From the .../source/test/testsuite directory run
# ./ClusterShutdownSuite.py -q ./<QAAutotestConfig.ini> -d <sudo_pwd> --verbose
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
import testcases.TestMgt
import NodeVerifyDownSuite


def suiteConstruction(self, action="kill-uninst"):
    """
    Construct the ordered set of test cases that comprise the
    Cluster Shutdown test suite.
    """
    suite = unittest.TestSuite()

    if action.count("remove") > 0:
        # One test case to remove the cluster services.
        suite.addTest(testcases.TestFDSSysMgt.TestNodeRemoveServices())

    if action.count("shutdown") > 0:
        # One test case to remove the cluster services.
        suite.addTest(testcases.TestFDSSysMgt.TestDomainShutdown())

    if action.count("kill") > 0:
        # One test case to shutdown the cluster.
        suite.addTest(testcases.TestFDSSysMgt.TestNodeKill())

        # Verify that all nodes are down.
        nodeDownSuite = NodeVerifyDownSuite.suiteConstruction(self=None)
        suite.addTest(nodeDownSuite)

        # Shutdown Redis.
        suite.addTest(testcases.TestFDSEnvMgt.TestShutdownRedis())
        suite.addTest(testcases.TestFDSEnvMgt.TestVerifyRedisDown())

    if action.count("uninst") > 0:
        # Cleanup FDS installation directory.
        suite.addTest(testcases.TestFDSEnvMgt.TestFDSDeleteInstDir())
        # This one will take care of other product artifacts such as SHM files.
        suite.addTest(testcases.TestFDSEnvMgt.TestFDSSelectiveInstDirClean())

    return suite

if __name__ == '__main__':
	
    # Handle FDS specific commandline arguments.
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

    test_suite = suiteConstruction(self=None)
    runner.run(test_suite)


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
import logging
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSModMgt
import testcases.TestFDSSysMgt
import testcases.TestFDSSysLoad
import testcases.TestMgt
import NodeWaitSuite


def suiteConstruction(self, action="installbootactivate"):
    """
    Construct the ordered set of test cases that install,
    boot, and activate a cluster defined in the input FDS
    Scenario config file, according to the specified action.
    """
    suite = unittest.TestSuite()

    # By constructing a generic test case at this point, we gain
    # access to the FDS config file from which
    # we'll pull node configuration.
    genericTestCase = testcases.TestCase.FDSTestCase()
    fdscfg = genericTestCase.parameters["fdscfg"]
    log = logging.getLogger("ClusterBootSuite")

    if action.count("install") > 0:
        # Build the necessary FDS infrastructure.
        suite.addTest(testcases.TestFDSEnvMgt.TestFDSCreateInstDir())
        suite.addTest(testcases.TestFDSEnvMgt.TestRestartRedisClean())

    if action.count("boot") > 0:
        # Start the the OM's PM.
        suite.addTest(testcases.TestFDSModMgt.TestPMForOMBringUp())
        suite.addTest(testcases.TestFDSModMgt.TestPMForOMWait())

        # Now start OM.
        suite.addTest(testcases.TestFDSModMgt.TestOMBringUp())
        suite.addTest(testcases.TestFDSModMgt.TestOMWait())

        # Start the remaining PMs
        suite.addTest(testcases.TestFDSModMgt.TestPMBringUp())
        suite.addTest(testcases.TestFDSModMgt.TestPMWait())

        # Give the nodes some time to initialize.
        suite.addTest(testcases.TestMgt.TestWait(delay=10, reason="to let the nodes initialize"))

    if action.count("activate") > 0:
        # Depending upon whether any node have specific services configured for them,
        # activation works differently.
        specifiedServices = False
        for n in fdscfg.rt_obj.cfg_nodes:
            if "services" in n.nd_conf_dict:
                specifiedServices = True
                break

        # Not doing TestClusterActivate for a multi-node
        # cluster allows us to avoid FS-879.
        if specifiedServices or (len(fdscfg.rt_obj.cfg_nodes) > 1):
            # Activate the cluster one node at a time with configured services.
            suite.addTest(testcases.TestFDSSysMgt.TestNodeActivate())
        else:
            # Activate the cluster with default services.
            suite.addTest(testcases.TestFDSSysMgt.TestClusterActivate())

        suite.addTest(testcases.TestMgt.TestWait(delay=10, reason="to let the cluster activate"))

    if (action.count("boot") > 0) or (action.count("activate") > 0):
        # Check that all nodes are up.
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
    # TODO(Greg) I've tried everything I can think of, but stop-on-fail/failfast does not
    # stop the suite upon first test case failure. So I've implemented our own
    # "failfast" via the TestCase.pyUnitTCFailure global. (And besides, some system
    # test users - most notably, Jenkins - were reporting that XMLTestRunner did not
    # recognize the failfast argument.)
    #runner = xmlrunner.XMLTestRunner(output=log_dir, failfast=failfast)
    runner = xmlrunner.XMLTestRunner(output=log_dir)

    test_suite = suiteConstruction(self=None)
    runner.run(test_suite)


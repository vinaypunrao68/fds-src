#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
# From the .../source/test/testsuite directory run
# ./DomainBootSuite.py -q ./<QAAutotestConfig.ini> -d <sudo_pwd> --verbose
#
# where
#  - <QAAutotestConfig.ini> - One of TwoNodeDomain.ini or FourNodeDomain.ini found in the
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
import testcases.TestFDSServiceMgt
import testcases.TestFDSSysMgt
import testcases.TestFDSSysLoad
import testcases.TestMgt
import NodeWaitSuite


def suiteConstruction(self, action="installbootactivate"):
    """
    Construct the ordered set of test cases that install,
    boot, and activate a domain defined in the input FDS
    Scenario config file, according to the specified action.
    """
    suite = unittest.TestSuite()

    # By constructing a generic test case at this point, we gain
    # access to the FDS config file from which
    # we'll pull node configuration.
    genericTestCase = testcases.TestCase.FDSTestCase()
    fdscfg = genericTestCase.parameters["fdscfg"]
    log = logging.getLogger("DomainBootSuite")

    # Note: If installing, restart Redis in a clean state. If booting
    # just boot Redis but don't mess with it's state.
    if action.count("install") > 0:
        # Build the necessary FDS infrastructure.
        if fdscfg.rt_obj.cfg_remote_nodes is not True:
            suite.addTest(testcases.TestFDSEnvMgt.TestFDSInstall())
        suite.addTest(testcases.TestFDSEnvMgt.TestRestartRedisClean())
        suite.addTest(testcases.TestFDSEnvMgt.TestVerifyRedisUp())
        suite.addTest(testcases.TestFDSEnvMgt.TestRestartInfluxDBClean())
        suite.addTest(testcases.TestFDSEnvMgt.TestVerifyInfluxDBUp())

    if action.count("boot_noverify") > 0:
        if action.count("install") == 0 and fdscfg.rt_obj.cfg_is_fds_installed is not True:
            suite.addTest(testcases.TestFDSEnvMgt.TestBootRedis())
            suite.addTest(testcases.TestFDSEnvMgt.TestVerifyRedisUp())
            suite.addTest(testcases.TestFDSEnvMgt.TestBootInfluxDB())
            suite.addTest(testcases.TestFDSEnvMgt.TestVerifyInfluxDBUp())

        # Start the the OM's PM.
        suite.addTest(testcases.TestFDSServiceMgt.TestPMForOMBringUp())

        # Now start OM.
        suite.addTest(testcases.TestFDSServiceMgt.TestOMBringUp())

        # Start the remaining PMs
        suite.addTest(testcases.TestFDSServiceMgt.TestPMBringUp())

        # Give the nodes some time to initialize.
        suite.addTest(testcases.TestMgt.TestWait(delay=10, reason="to let the nodes initialize"))
    elif action.count("boot") > 0:
        if action.count("install") == 0:
            suite.addTest(testcases.TestFDSEnvMgt.TestBootRedis())
            suite.addTest(testcases.TestFDSEnvMgt.TestVerifyRedisUp())
            suite.addTest(testcases.TestFDSEnvMgt.TestBootInfluxDB())
            suite.addTest(testcases.TestFDSEnvMgt.TestVerifyInfluxDBUp())

        # Start the the OM's PM.
        suite.addTest(testcases.TestFDSServiceMgt.TestPMForOMBringUp())
        suite.addTest(testcases.TestFDSServiceMgt.TestPMForOMWait())

        # Now start OM.
        suite.addTest(testcases.TestFDSServiceMgt.TestOMBringUp())
        suite.addTest(testcases.TestFDSServiceMgt.TestOMWait())

        # Start the remaining PMs
        suite.addTest(testcases.TestFDSServiceMgt.TestPMBringUp())
        suite.addTest(testcases.TestFDSServiceMgt.TestPMWait())

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

        if specifiedServices:
            # Activate the domain one node at a time with configured services.
            suite.addTest(testcases.TestFDSSysMgt.TestNodeActivate())
        else:
            # Activate the domain with default services.
            suite.addTest(testcases.TestFDSSysMgt.TestDomainActivateServices())

        suite.addTest(testcases.TestMgt.TestWait(delay=10, reason="to let the domain activate"))

    if action.count("graceful_restart") > 0:
        # This restarts the domain after graceful shutdown
        suite.addTest(testcases.TestFDSSysMgt.TestDomainStartup())

    if ((action.count("boot") > 0) or (action.count("activate") > 0)) and (action.count("noverify") == 0):
        # Check that all nodes are up.
        nodeUpSuite = NodeWaitSuite.suiteConstruction(self=None)
        suite.addTest(nodeUpSuite)

    return suite

if __name__ == '__main__':
	
    # Handle FDS specific commandline arguments.
    log_dir, failfast, install, reusecluster, pyUnitConfig = testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)

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


#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
#
# From the .../source/test/testsuite directory run
# ./DomainShutdownSuite.py -q ./<QAAutotestConfig.ini> -d <sudo_pwd> --verbose
#
# where
#  - <QAAutotestConfig.ini> - One of TwoNodeDomain.ini or FourNodeDomain.ini found in the
#  testsuite directory, or one of your own.
#  - <sudo_pwd> - The password you use for executing sudo on your machine. Even
#  if your machine does not require a password for sudo, you must provide something, e.g. "dummy".
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSServiceMgt
import testcases.TestFDSSysMgt
import testcases.TestFDSSysLoad
import testcases.TestMgt
import NodeVerifyDownSuite


def suiteConstruction(self, action="kill-uninst"):
    """
    Construct the ordered set of test cases that comprise the
    Domain Shutdown test suite.
    """
    suite = unittest.TestSuite()
    genericTestCase = testcases.TestCase.FDSTestCase()
    fdscfg = genericTestCase.parameters["fdscfg"]


    if action.count("remove") > 0:
        # One test case to remove the domain services.
        suite.addTest(testcases.TestFDSSysMgt.TestNodeRemoveServices())

    if action.count("shutdown") > 0:
        # One test case to remove the domain services.
        suite.addTest(testcases.TestFDSSysMgt.TestDomainShutdown())

    if action.count("kill") > 0:
        # One test case to shutdown the domain.
        suite.addTest(testcases.TestFDSSysMgt.TestNodeKill())

        # Shutdown Redis and InfluxDB.
        suite.addTest(testcases.TestFDSEnvMgt.TestShutdownRedis())
        suite.addTest(testcases.TestFDSEnvMgt.TestShutdownInfluxDB())

        # Verify down unless requested not to.
        if action.count("kill_noverify") == 0:
            # Verify that all nodes are down.
            nodeDownSuite = NodeVerifyDownSuite.suiteConstruction(self=None)
            suite.addTest(nodeDownSuite)

            # Verify operational DBs are down.
            suite.addTest(testcases.TestFDSEnvMgt.TestVerifyRedisDown())
            suite.addTest(testcases.TestFDSEnvMgt.TestVerifyInfluxDBDown())

    if action.count("uninst") > 0:
        if fdscfg.rt_obj.cfg_remote_fds_deploy is not True:
            # Cleanup FDS installation directory.
            suite.addTest(testcases.TestFDSEnvMgt.TestFDSDeleteInstDir())
            # This one will take care of other product artifacts such as SHM files.
            suite.addTest(testcases.TestFDSEnvMgt.TestFDSSelectiveInstDirClean())
        else:
            # We add kill-uninst scenario only if it is last scenario in cfg file because deployment is done only once even before first scenario runs TODO: Pooja
            scenarios = fdscfg.rt_get_obj('cfg_scenarios')
            number_of_scenarios = len(scenarios)
            last_scenario = scenarios[number_of_scenarios-1]
            for scenario in scenarios:
                if "action" in scenario.nd_conf_dict:
                    action = scenario.nd_conf_dict['action']
                    if (action.count("uninst") > 0) and scenario == last_scenario:
                        suite.addTest(testcases.TestFDSEnvMgt.TestFDSTeardownDomain())

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

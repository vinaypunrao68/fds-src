#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
#
# This test suite dynamically constructs itself (that is,
# identifies the test cases from which a suite is derived)
# from "scenario" sections in the provided FDS
# configuration file. Therefore, those test cases that it
# runs are completely determined by the FDS configuration
# file.
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSServiceMgt
import testcases.TestFDSSysMgt
import testcases.TestFDSSysLoad
import testcases.TestFDSSysVerify
import testcases.TestFDSVolMgt
import testcases.TestMgt
import logging

log_dir = None

def suiteConstruction(self, install):
    """
    Construct the ordered set of test cases that comprise the
    test suite defined by the input FDS scenario config file.
    """
    suite = unittest.TestSuite()

    # By constructing a generic test case at this point, we gain
    # access to the FDS config file from which
    # we'll pull the scenarios used to identify test cases.
    genericTestCase = testcases.TestCase.FDSTestCase()
    fdscfg = genericTestCase.parameters["fdscfg"]
    log = logging.getLogger()

    # Pull the scenarios from the FDS config file. These
    # will be mapped to test cases.
    scenarios = fdscfg.rt_get_obj('cfg_scenarios')

    # If we don't have any scenarios someone screwed up
    # the FDS config file.
    if len(scenarios) == 0:
        log.error("Your FDS config file, %s, must specify scenario sections." %
                  genericTestCase.parameters["fds_config_file"])
        sys.exit(1)

    for scenario in scenarios:
        testcases.TestMgt.queue_up_scenario(suite=suite, scenario=scenario, log_dir=log_dir, install_done = install)

    return suite


if __name__ == '__main__':

    # Handle FDS specific commandline arguments.
    log_dir, failfast, install, reusecluster = testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    # If a test log directory was not supplied on the command line (with option "-l"),
    # then default it.
    if log_dir is None:
        log_dir = 'test-reports'

    test_suite = suiteConstruction(self=None, install= install)

    #For remote nodes, if reusecluster is false then tear down the domain
    if install is True and reusecluster is False:
        test_suite.addTest(testcases.TestFDSEnvMgt.TestFDSTeardownDomain())
    # Get a test runner that will output an xUnit XML report for Jenkins
    # TODO(Greg) I've tried everything I can think of, but stop-on-fail/failfast does not
    # stop the suite upon first test case failure. So I've implemented our own
    # "failfast" via the TestCase.pyUnitTCFailure global. (And besides, some system
    # test users - most notably, Jenkins - were reporting that XMLTestRunner did not
    # recognize the failfast argument.)
    #runner = xmlrunner.XMLTestRunner(output=log_dir, failfast=failfast)
    #runner = xmlrunner.XMLTestRunner(output=log_dir, verbosity=0)
    runner = testcases.TestMgt.FDSTestRunner(output=log_dir, verbosity=0,
                                             fds_logger=logging.getLogger())
    runner.run(test_suite)


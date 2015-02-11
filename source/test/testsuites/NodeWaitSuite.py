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

def suiteConstruction(self, fdsNodes=None):
    """
    Construct the ordered set of test cases that comprise the
    test cases necessary to check whether a node is started.
    """
    suite = unittest.TestSuite()

    # Check the node(s) according to configuration supplied with the -q cli option
    # or if a list of nodes is provided, check them specifically.
    if fdsNodes is not None:

        # By constructing a generic test case at this point, we gain
        # access to the FDS config file from which
        # we'll pull the node configurations to determine the context
        # for verification.
        genericTestCase = testcases.TestCase.FDSTestCase()
        fdscfg = genericTestCase.parameters["fdscfg"]

        for node in fdsNodes:
            # If it's the OM node, check it's PM and OM. Otherwise,
            # we're looking for a non-OM PM.
            if node.nd_conf_dict['node-name'] == fdscfg.rt_om_node.nd_conf_dict['node-name']:
                suite.addTest(testcases.TestFDSModMgt.TestPMForOMWait(node=node))
                suite.addTest(testcases.TestFDSModMgt.TestOMWait(node=node))
            else:
                suite.addTest(testcases.TestFDSModMgt.TestPMWait(node=node))

            suite.addTest(testcases.TestFDSModMgt.TestDMWait(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestSMWait(node=node))
            suite.addTest(testcases.TestFDSModMgt.TestAMWait(node=node))
    else:
        suite.addTest(testcases.TestFDSModMgt.TestPMForOMWait())
        suite.addTest(testcases.TestFDSModMgt.TestOMWait())
        suite.addTest(testcases.TestFDSModMgt.TestPMWait())
        suite.addTest(testcases.TestFDSModMgt.TestDMWait())
        suite.addTest(testcases.TestFDSModMgt.TestSMWait())
        suite.addTest(testcases.TestFDSModMgt.TestAMWait())

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


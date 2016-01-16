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
import testcases.TestFDSSysLoad
import DomainBootSuite
import NodeWaitSuite
import BotoBLOBSuite
import NodeResilienceSuite
import BlockBlobSuite

def suiteConstruction(self):
    """
    Construct the ordered set of test cases that comprise the
    Build Smoke Test suite.
    """
    suite = unittest.TestSuite()

    # Build the necessary FDS infrastructure and boot the domain
    # according to configuration.
    domainBootSuite = DomainBootSuite.suiteConstruction(self=None)
    suite.addTest(domainBootSuite)

    # Load test.
    suite.addTest(testcases.TestFDSSysLoad.TestSmokeLoad())

    # Small/Large BLOB test using Boto.
    blobSuite = BotoBLOBSuite.suiteConstruction(self=None)
    suite.addTest(blobSuite)

    # Everyone should still be up.
    nodeUpSuite = NodeWaitSuite.suiteConstruction(self=None)
    suite.addTest(nodeUpSuite)

    # Block Blob test.
    blockSuite = BlockBlobSuite.suiteConstruction(self=None)
    suite.addTest(blockSuite)

    # Everyone should still be up.
    suite.addTest(nodeUpSuite)

    # Node Resiliency suite.
    nodeResilienceSuite = NodeResilienceSuite.suiteConstruction(self=None)
    suite.addTest(nodeResilienceSuite)

    suite.addTest(testcases.TestFDSSysMgt.TestNodeKill())

    # Cleanup FDS installation directory.
    suite.addTest(testcases.TestFDSEnvMgt.TestFDSDeleteInstDir())

    return suite

if __name__ == '__main__':
	
    # Handle FDS specific commandline arguments.
    log_dir, failfast, install, reusecluster, config = testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)

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


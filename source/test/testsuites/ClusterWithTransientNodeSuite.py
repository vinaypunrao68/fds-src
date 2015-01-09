#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
# This test suite checks correctness in booting a
# cluster and then adding/removing another node. In particular
# we wish to see that Data Management Tables are properly deployed
# upon the nodes in the cluster.
#
# Note that the scenario configuration file must specify at
# least one transient node.
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
import TransientNodeWaitSuite


def suiteConstruction():
    """
    Construct the ordered set of test cases that comprise the
    Transient Node Cluster test suite.
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

    # Given the nodes some time to initialize.
    suite.addTest(testcases.TestFDSSysMgt.TestWait())

    # Activate the cluster.
    suite.addTest(testcases.TestFDSSysMgt.TestClusterActivate())

    # Bring up any AMs.
    suite.addTest(testcases.TestFDSModMgt.TestAMBringup())

    # Check that all non-transient nodes are up.
    nodeUpSuite = NodeWaitSuite.suiteConstruction()
    suite.addTest(nodeUpSuite)

    # Give the nodes some time to initialize.
    suite.addTest(testcases.TestFDSSysMgt.TestWait())

    # Verify that the OM log indicates successful DMT migration to this point.
    suite.addTest(testcases.TestFDSSysMgt.TestInitial1NodeDMTMigration())

    # Now bring up transient nodes.
    suite.addTest(testcases.TestFDSModMgt.TestPMForTransientBringUp())
    suite.addTest(testcases.TestFDSModMgt.TestPMForTransientWait())

    # Give the transient PMs some time to initialize.
    suite.addTest(testcases.TestFDSSysMgt.TestWait())

    # Activate the cluster to include the transient nodes.
    suite.addTest(testcases.TestFDSSysMgt.TestClusterActivate())

    # Check that *all* nodes are up.
    suite.addTest(nodeUpSuite)
    transientNodeUpSuite = TransientNodeWaitSuite.suiteConstruction()
    suite.addTest(transientNodeUpSuite)

    # Give the transient nodes some time to initialize.
    suite.addTest(testcases.TestFDSSysMgt.TestWait())

    # Verify that the OM log indicates successful DMT migration to this point.
    suite.addTest(testcases.TestFDSSysMgt.TestTransientAddNodeDMTMigration())

    # Remove the services of the transient nodes.
    suite.addTest(testcases.TestFDSSysMgt.TestTransientRemoveService())

    # Give the OM some time to manage the node removal.
    suite.addTest(testcases.TestFDSSysMgt.TestWait())

    # Verify that the OM log indicates successful DMT migration to this point.
    suite.addTest(testcases.TestFDSSysMgt.TestTransientRemoveNodeDMTMigration())

    # Since out cluster is all on one machine, this will actually shutdown the cluster.
    suite.addTest(testcases.TestFDSSysMgt.TestNodeShutdown())

    # Cleanup FDS installation directory.
    suite.addTest(testcases.TestFDSEnvMgt.TestFDSDeleteInstDir())

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

    test_suite = suiteConstruction()
    runner.run(test_suite)


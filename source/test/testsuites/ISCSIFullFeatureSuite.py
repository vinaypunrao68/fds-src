#!/usr/bin/python
#
# Copyright 2016 by Formation Data Systems, Inc.
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSServiceMgt
import testcases.TestFDSSysMgt
import testcases.TestISCSIIntFace
import NodeWaitSuite

def suiteConstruction(self):
    """
    Construct the ordered set of test cases that comprise the
    test cases necessary to test iSCSI full feature list.
    """
    suite = unittest.TestSuite()

    # Check that all nodes are up.
    nodeUpSuite = NodeWaitSuite.suiteConstruction(self=None)
    suite.addTest(nodeUpSuite)

    # Create iSCSI volume
    suite.addTest(testcases.TestISCSIIntFace.TestISCSICrtVolume())

    # Test a list volumes
    suite.addTest(testcases.TestISCSIIntFace.TestISCSIListVolumes(None, 'volISCSI'))

    # Save target name to fixture
    suite.addTest(testcases.TestISCSIIntFace.TestISCSIDiscoverVolume())

    # Attach iSCSI device
    initiator_name = 'initiator1'
    suite.addTest(testcases.TestISCSIIntFace.TestISCSIAttachVolume(initiator_name,
        None, None, 'volISCSI'))

    # Need a file system...
    suite.addTest(testcases.TestISCSIIntFace.TestISCSIMakeFilesystem(None, None, 'volISCSI'))

    # Use the char device interface
    suite.addTest(testcases.TestISCSIIntFace.TestISCSIUnitReady(None, None, 'volISCSI'))

    # This is where we will add char device interface tests, if they make sense,
    # for all of:

    # REPORT_LUNS
    # MODE_SENSE
    # INQUIRY
    # FORMAT_UNIT
    # PERSISTENT_RESERVATION_IN
    # PERSISTENT_RESERVATION_OUT
    # RESERVE_6
    # RELEASE_6
    # READ_CAPACITY
    # READ_6
    # READ_10
    # READ_12
    # READ_16
    # WRITE_6
    # WRITE_10
    # WRITE_12
    # WRITE_16

    # Run an fio sequential write workload
#    suite.addTest(testcases.TestISCSIIntFace.TestBlockFioSeqW())

    # Run an fio random write workload
#    suite.addTest(testcases.TestISCSIIntFace.TestBlockFioRandW())

    # Run an fio read/write workload
#    suite.addTest(testcases.TestISCSIIntFace.TestBlockFioRW())

    # Run an fio random read/write workload
#    suite.addTest(testcases.TestISCSIIntFace.TestBlockFioRandRW())

    # Detach iSCSI volume by logging out of iSCSI node record.
    # Also cleans up by deleting iSCSI node record.
    suite.addTest(testcases.TestISCSIIntFace.TestISCSIDetachVolume(None, None, 'volISCSI'))

    return suite

if __name__ == '__main__':

    # Handle FDS specific commandline arguments.
    log_dir, failfast, install, reusecluster, config = testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)

    # If a test log directory was not supplied on the command line (with option "-l"),
    # then default it.
    if log_dir is None:
        log_dir = 'test-reports'

    # Get a test runner that will output an xUnit XML report for Jenkins
    runner = xmlrunner.XMLTestRunner(output=log_dir)

    test_suite = suiteConstruction(self=None)

    runner.run(test_suite)


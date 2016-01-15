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
import testcases.TestISCSIIntFace
import NodeWaitSuite

def suiteConstruction(self):
    """
    Construct the ordered set of test cases that comprise the
    test cases necessary to test handling a large BLOB.
    """
    suite = unittest.TestSuite()

    # Check that all nodes are up.
    nodeUpSuite = NodeWaitSuite.suiteConstruction(self=None)
    suite.addTest(nodeUpSuite)

    # Create iSCSI volume
    suite.addTest(testcases.TestISCSIIntFace.TestISCSICrtVolume())

    # Test a list volumes to a block device
#    suite.addTest(testcases.TestBlockIntFace.TestBlockListVolumes())

    # TODO: return list of iSCSI names
    # TODO: call with a list of volume names
    suite.addTest(testcases.TestISCSIIntFace.TestISCSIDiscoverVolume())

    # Attach iSCSI device
    suite.addTest(testcases.TestISCSIIntFace.TestISCSIAttachVolume(None, None, 'volISCSI'))

    # Run an fio sequential write workload
#    suite.addTest(testcases.TestBlockIntFace.TestBlockFioSeqW())

    # Run an fio random write workload
#    suite.addTest(testcases.TestBlockIntFace.TestBlockFioRandW())

    # Run an fio read/write workload
#    suite.addTest(testcases.TestBlockIntFace.TestBlockFioRW())

    # Run an fio random read/write workload
#    suite.addTest(testcases.TestBlockIntFace.TestBlockFioRandRW())

    # Detach a block device
#    suite.addTest(testcases.TestBlockIntFace.TestBlockDetachVolume())

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


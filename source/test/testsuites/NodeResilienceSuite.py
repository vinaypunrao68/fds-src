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
import testcases.TestMgt
import testcases.TestOMIntFace
import testcases.TestS3IntFace
import NodeWaitSuite

def suiteConstruction(self):
    """
    Construct the ordered set of test cases that check the
    resiliency of a node as components are stopped and started.
    """
    suite = unittest.TestSuite()

    # Check that all nodes are up.
    nodeUpSuite = NodeWaitSuite.suiteConstruction(self=None)
    suite.addTest(nodeUpSuite)

    # Create a S3 bucket and object that we can use to make sure
    # things look good after components are recycled.
    #
    # For the S3 interface, we need an authorization token from OM.
    suite.addTest(testcases.TestOMIntFace.TestGetAuthToken())

    # Create the S3 connection we will use.
    suite.addTest(testcases.TestS3IntFace.TestS3GetConn())

    # Create an S3 bucket.
    suite.addTest(testcases.TestS3IntFace.TestS3CrtBucket())

    # Load a 'largish' BLOB (<= 2MiB) into the bucket with meta-data.
    suite.addTest(testcases.TestS3IntFace.TestS3LoadMBLOB())

    # Run through the components, recycling them as we go, making sure
    # everything is back up when expected, then make sure we still have
    # our bucket and BLOB and meta-data.
    #
    # Currently (12/16/2014) the OM is not very resilient.
    #suite.addTest(testcases.TestFDSModMgt.TestOMKill())
    #suite.addTest(testcases.TestFDSModMgt.TestOMVerifyDown())
    #suite.addTest(testcases.TestFDSModMgt.TestOMBringUp())
    #suite.addTest(testcases.TestFDSModMgt.TestOMWait())

    #suite.addTest(testcases.TestFDSModMgt.TestAMKill())
    #suite.addTest(testcases.TestFDSModMgt.TestAMVerifyDown())
    #suite.addTest(testcases.TestFDSModMgt.TestAMBringUp())
    #suite.addTest(testcases.TestFDSModMgt.TestAMWait())

    suite.addTest(testcases.TestFDSServiceMgt.TestSMKill())
    suite.addTest(testcases.TestFDSServiceMgt.TestSMVerifyDown())
    suite.addTest(testcases.TestFDSServiceMgt.TestSMWait())

    #suite.addTest(testcases.TestFDSModMgt.TestDMKill())
    #suite.addTest(testcases.TestFDSModMgt.TestDMVerifyDown())
    #suite.addTest(testcases.TestFDSModMgt.TestDMBringUp())
    #suite.addTest(testcases.TestFDSModMgt.TestDMWait())

    suite.addTest(testcases.TestFDSServiceMgt.TestPMKill())
    suite.addTest(testcases.TestFDSServiceMgt.TestPMVerifyDown())
    suite.addTest(testcases.TestFDSServiceMgt.TestPMBringUp())
    suite.addTest(testcases.TestFDSServiceMgt.TestPMWait())

    # Verify everyone is up.
    suite.addTest(nodeUpSuite)

    # Given the node some time to initialize.
    suite.addTest(testcases.TestMgt.TestWait())

    # Now verify we still have our data.
    suite.addTest(testcases.TestS3IntFace.TestS3VerifyMBLOB())

    # Add another for 'grins'.
    suite.addTest(testcases.TestS3IntFace.TestS3LoadFBLOB())

    # Delete the keys of the bucket.
    suite.addTest(testcases.TestS3IntFace.TestS3DelBucketKeys())

    # Delete the S3 bucket.
    suite.addTest(testcases.TestS3IntFace.TestS3DelBucket())

    return suite

if __name__ == '__main__':

    # Handle FDS specific commandline arguments.
    log_dir, failfast = testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)

    # If a test log directory was not supplied on the command line (with option "-l"),
    # then default it.
    if log_dir is None:
        log_dir = 'test-reports'

    # Get a test runner that will output an xUnit XML report for Jenkins
    runner = xmlrunner.XMLTestRunner(output=log_dir)

    test_suite = suiteConstruction(self=None)

    runner.run(test_suite)


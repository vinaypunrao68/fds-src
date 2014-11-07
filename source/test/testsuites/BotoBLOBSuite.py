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
import testcases.TestOMIntFace
import testcases.TestS3IntFace
import NodeWaitSuite

def suiteConstruction():
    """
    Construct the ordered set of test cases that comprise the
    test cases necessary to test handling a large BLOB.
    """
    suite = unittest.TestSuite()

    # Check that all nodes are up.
    nodeUpSuite = NodeWaitSuite.suiteConstruction()
    suite.addTest(nodeUpSuite)

    # For the S3 interface, we need an authorization token from OM.
    suite.addTest(testcases.TestOMIntFace.TestGetAuthToken())

    # Create the S3 connection we will use.
    suite.addTest(testcases.TestS3IntFace.TestS3GetConn())

    # Create an S3 bucket.
    suite.addTest(testcases.TestS3IntFace.TestS3CrtBucket())

    # Load a 'small' BLOB (<= 2MiB) into the bucket.
    suite.addTest(testcases.TestS3IntFace.TestS3LoadSBLOB())

    # Load a 'largish' BLOB (<= 2MiB) into the bucket.
    suite.addTest(testcases.TestS3IntFace.TestS3LoadFBLOB())

    # Load a 'largish' BLOB (<= 2MiB) into the bucket with meta-data.
    suite.addTest(testcases.TestS3IntFace.TestS3LoadMBLOB())

    # Load a 'large' BLOB (> 2MiB) into the bucket.
    suite.addTest(testcases.TestS3IntFace.TestS3LoadLBLOB())

    # List the keys of the bucket.
    suite.addTest(testcases.TestS3IntFace.TestS3ListBucketKeys())

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

    test_suite = suiteConstruction()

    runner.run(test_suite)


#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSServiceMgt as service
import testcases.TestFDSSysMgt
import testcases.TestMgt
from testcases.TestMgt import TestWait 
import testcases.TestOMIntFace
from testcases.TestOMIntFace import TestGetAuthToken 
import testcases.TestS3IntFace as s3
import NodeWaitSuite

def suiteConstruction(self):
    """
    Construct the ordered set of test cases that check the
    resiliency of a node as components are stopped and started.
    """
    suite = unittest.TestSuite()

    # -- setup the connection
    tests = [        
        TestGetAuthToken()                     ,
        s3.TestS3GetConn()                     ,
        s3.TestS3CrtBucket()                   ,
        TestWait(delay=10)                     ,

        s3.TestPuts()                          ,
        s3.TestGets()                          ,

        s3.TestPuts(dataset='2' )              ,
        s3.TestGets(dataset='2')               ,
        #s3.TestDeletes(dataset='2')           ,
        #s3.TestKeys(dataset='2', exist=False) ,
    ]    
    suite.addTests(tests)

    for svc in ['DM', 'SM', 'AM']:
        tests = [   
            eval('service.Test'+ svc + 'Kill()')   ,
            eval('service.Test'+ svc + 'Wait()')   ,
            TestWait(delay=10)                     ,
            s3.TestGets()                          ,

            #s3.TestPuts(dataset='2' )             ,
            #s3.TestGets(dataset='2')              ,
            #s3.TestDeletes(dataset='2')           ,
            #s3.TestKeys(dataset='2', exist=False) ,
        ]
        suite.addTests(tests)

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


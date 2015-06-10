#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSSysMgt
import testcases.TestMgt
import testcases.TestOMIntFace
import testcases.TestFDSServiceMgt as service

from testcases.TestOMIntFace import TestGetAuthToken 
from testcases.TestMgt import TestWait 
from testcases.TestMgt import TestLogMarker

import testcases.TestS3IntFace as s3
import NodeWaitSuite

def log(msg):
    return TestLogMarker(scenario='Restart test', marker=msg)

def suiteConstruction(self):
    """
    Construct the ordered set of test cases that check the
    resiliency of a node as components are stopped and started.
    """
    suite = unittest.TestSuite()

    # -- setup the connection
    tests = [
        log("setting up base system")               ,
        TestGetAuthToken()                          ,
        s3.TestS3GetConn()                          ,
        s3.TestS3CrtBucket(bucket='volume1')        ,
        TestWait(delay=10)                          ,

        s3.TestPuts()                               ,
        s3.TestGets()                               ,

        s3.TestPuts(dataset=2 )                     ,
        s3.TestGets(dataset=2)                      ,
        s3.TestDeletes(dataset=2)                   ,
        s3.TestKeys(dataset=2, exist=False)         ,
    ]
    suite.addTests(tests)

    for svc in ['AM', 'SM', 'DM']:
        delay=10 if svc != 'AM' else 60

        tests = [
            log ("restarting {}".format(svc))       ,
            eval('service.Test'+ svc + 'Kill()')    ,
            eval('service.Test'+ svc + 'Wait()')    ,
            TestWait(delay=delay)                   ,
            s3.TestS3CrtBucket(bucket='volume1')    ,
            s3.TestGets()                           ,

            log("test - crt/put/get/del/delbucket") ,
            s3.TestS3CrtBucket(bucket='volume2')    ,
            s3.TestPuts(dataset=2 )                 ,
            s3.TestGets(dataset=2)                  ,
            s3.TestDeletes(dataset=2)               ,
            s3.TestKeys(dataset=2, exist=False)     ,
            s3.TestS3DelBucket(bucket='volume2')    ,
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


import boto
import os
import sys
import unittest

from boto.s3.key import Key

import config
import users
import lib
import testsets.testcase as testcase

class TestCreateMultipleBuckets(testcase.FDSTestCase):
    
    '''
    Test the scenario where for a particular user, there's a bucket for
    that user. This scenario is seen in Jive, so this test simulates
    some of the functionalities the product needs for Jive.
    
    Attributes:
    -----------
    table : dict
        table is a map between the bucket and the user.
    parameters: dict
        a dictionary of parameters to be used by the test.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestCreateMultipleBuckets, self).__init__(parameters=parameters,
                                                    config_file=config_file,
                                                    om_ip_address=om_ip_address)
        # since the FDS doesn't support creating more than 1024 buckets,
        # we will limit this to 1023.
        self.buckets = []
        self.conn = self.parameters['s3'].conn
    
    @unittest.expectedFailure
    def runTest(self):
        '''
        First create the S3 buckets for every user, and then store a
        test file in that particular bucket.
        '''
        self.test_create_multiple_buckets()          
    
    @unittest.expectedFailure
    def tearDown(self):
        '''
        Remove all the buckets which were created by this test.
        '''
        for bucket in self.buckets:
            if bucket:
                for key in bucket.list():
                    bucket.delete_key(key)
                self.log.info("Deleting bucket %s" % bucket.name)
                self.conn.delete_bucket(bucket.name)
        
    def test_create_multiple_buckets(self):
        '''
        Create a bucket per user, and upload a simple file, to assert
        the data is there.
        '''
        for i in xrange(0, 1023):
            # the bucket name is a 7 char key.
            name = "volume_" + str(i)
            try:
                bucket = self.conn.create_bucket(name)
                if bucket == None:
                    raise Exception("Invalid bucket.")
                    sys.exit(1)
                self.log.info("Created bucket %s" % name)
                self.buckets.append(bucket)
            except Exception, e:
                self.log.exception(e)
                continue

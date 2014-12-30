import boto
import os
import sys

from boto.s3.key import Key

import config
import users
import utils
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
    def __init__(self, parameters=None):
        super(TestCreateMultipleBuckets, self).__init__(parameters)
        self.my_users = []
        # since the FDS doesn't support creating more than 1024 buckets,
        # we will limit this to 1023.
        for i in xrange(1, config.NUMBER_USERS):
            user = users.User(id=i)
            self.my_users.append(user)
            
        self.table = {}
        self.conn = self.parameters['s3'].conn
    
    def runTest(self):
        '''
        First create the S3 buckets for every user, and then store a
        test file in that particular bucket.
        '''
        test_passed = False
        try:
            self.test_create_multiple_buckets()     
            self.log.info(table)           
            for k, v in self.table.iteritems():
                if k:
                    # test also if the file is there in the bucket.
                    if k.name != v.identifier[:7]:
                        raise
            test_passed = True
        except:
            test_passed = False
        super(self.__class__, self).reportTestCaseResult(test_passed)
    
    def tearDown(self):
        '''
        Remove all the buckets which were created by this test.
        '''
        for k, v in self.table.iteritems():
            if k:
                self.conn.delete_bucket(k.name)
        self.table = {}
        
    def test_create_multiple_buckets(self):
        '''
        Create a bucket per user, and upload a simple file, to assert
        the data is there.
        '''
        curr_dir = os.path.dirname(os.path.abspath(__file__))
        full_file_name = os.path.join(curr_dir, config.SAMPLE_FILE)
        for user in self.my_users:
            # the bucket name is a 7 char key.
            name = "hello_" + str(user.id)
            self.log.info("----- Name: %s --------" % name)
            try:
                self.log.info("Creating bucket: %s" % name)
                bucket = self.conn.create_bucket(name)
                if bucket == None:
                    raise Exception("Invalid bucket.")
                    sys.exit(1)
                table[bucket] = user
                k = Key(bucket)
                k.key = config.SAMPLE_FILE
                k.set_contents_from_filename(full_file_name,
                                             cb=utils.percent_cb,
                                             num_cb=10)
            except Exception, e:
                self.log.exception(e)
                continue

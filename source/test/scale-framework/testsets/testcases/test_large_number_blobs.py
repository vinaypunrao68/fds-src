#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
# author: Heang Lim
# email: heang@formationds.com
# file: test_multiple_block_mount_points.py

import random
import config
import config_parser
import s3
import utils
import testsets.testcase as testcase

class TestLargeNumberBlobs(testcase.FDSTestCase):
    
    '''
    This test produce a large number of data files, using the following data
    sizes:
    2,4,8,16,32,64,128,256,512
    and also produce 10 random data sizes, which indented to explore corner
    cases.
    '''
    # max number of blobs in each bucket
    MAX_NUMBER_BLOBS = 10000
    
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestLargeNumberBlobs, self).__init__(parameters=parameters,
                                                    config_file=config_file,
                                                    om_ip_address=om_ip_address)
        
        self.ip_addresses = config_parser.get_ips_from_inventory(
                                                            self.inventory_file)
        
        self.blob_sizes = set([2,4,8,16,32,64,128,256,512])
        samples = list(xrange(0, 1024))
        random_samples = random.sample(samples, 10)
        self.blob_sizes.union(random_samples)
        self.buckets = []
        
    def runTest(self):
        self.create_volumes()

    def tearDown(self):
        pass

    def connect_s3(self):
        '''
        establish a S3 connection
        '''
        s3conn = s3.S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            None,
            self.om_ip_address,
            config.FDS_S3_PORT,
            self.om_ip_address,
        )
        s3conn.s3_connect()
        return s3conn
    
    def generate_blob_size(self):
        pass
    
    def store_file_to_volume(self, bucket):
        pass

    def create_volumes(self):
        bucket_name = "volume_blob_size_%s"
        s3conn = self.connect_s3()
        for size in self.blob_sizes:
            bucket = s3conn.conn.create_bucket(bucket_name % size)
            
            if bucket == None:
                raise Exception("Invalid bucket.")
                # We won't be waiting for it to complete, short circuit it.
                self.test_passed = False
                self.reportTestCaseResult(self.test_passed)
            
            self.log.info("Volume %s created..." % bucket.name)
            self.buckets.append(bucket)
            self.store_file_to_volume(bucket)
            
            # create the volume for that particular name.
            for i in xrange(0, self.MAX_NUMBER_BLOBS):
                # copy the blob size to the bucket
                pass
            
    
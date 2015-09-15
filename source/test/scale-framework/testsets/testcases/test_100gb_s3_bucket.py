#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
# author: Philippe Ribeiro
# email: philippe@formationds.com
# file: test_100gb_s3_bucket.py

import lib
import config
import unittest
import requests
import json
import sys

import ssh
import os
from boto.s3.key import Key
import testsets.testcase as testcase


class Test100GBS3Bucket(testcase.FDSTestCase):
    '''
    The test also create another S3 bucket, and attempts to upload up to 100GB
    of data via S3.
    After that, add more 2GB of data, to ensure correctness.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(Test100GBS3Bucket, self).__init__(parameters=parameters,
                                               config_file=config_file,
                                               om_ip_address=om_ip_address)
    
    def runTest(self):
        self.upload_s3_volume()
    
    def upload_s3_volume(self):
        lib.create_dir(config.TEST_DIR)
        # lib.create_dir(config.DOWNLOAD_DIR)
        s3conn = lib.create_s3_connection(self.om_ip_address, self.om_ip_address)
        """
        Test is in debug mode due to the issue #FS-1384
        Once that issue is updated, test will be modified to write up to 100GB
        """
        bucket_name = "volume_100gb_s3_bucket"
        bucket = s3conn.conn.create_bucket(bucket_name)
        if bucket == None:
            raise Exception("Invalid bucket.")
            # We won't be waiting for it to complete, short circuit it.
            self.test_passed = False
            self.reportTestCaseResult(self.test_passed)
        self.log.info("Volume %s created..." % bucket.name)
        # Store all the files created to the bucket
        self.store_file_to_volume(bucket)
        # self.log.info("Trying to Download the blobs now.")
        # self.download_files(bucket)
        # self.check_files_hash()
        # Delete the bucket
        # self.destroy_volume(bucket, s3conn)
        # remove the existing file
        lib.remove_dir(config.DOWNLOAD_DIR)
        
    def store_file_to_volume(self, bucket):
        '''
        Given the list of files to be uploaded, presented in sample_files list,
        upload them to the corresponding volume
        
        Attributes:
        -----------
        bucket : bucket
            the S3 bucket (volume) where the data files will to uploaded to.
        '''
        # add the data files to the bucket.
        k = Key(bucket)
        sample = "test_sample_200M"
        path = os.path.join(config.TEST_DIR, sample)
        self.log.info("Uploading: %s" % path)
        if os.path.exists(path):
            k.key = sample
            k.set_contents_from_filename(path,
                                         cb=lib.percent_cb,
                                         num_cb=10)
            self.log.info("Uploaded file %s to bucket %s" % 
                         (sample, bucket.name))
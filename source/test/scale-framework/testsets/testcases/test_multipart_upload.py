#########################################################################
# @Copyright: 2015 by Formation Data Systems, Inc.                      #
# @file: test_multipart_upload.py                                                 #
# @author: Philippe Ribeiro                                             #
# @email: philippe@formationds.com                                      #
#########################################################################
import boto
import concurrent.futures
import config
import config_parser
import hashlib
import math
import os
import random
import subprocess
import shutil
import sys
import time
import unittest

from boto.s3.connection import OrdinaryCallingFormat
from boto.s3.key import Key
from boto.s3.bucket import Bucket
from filechunkio import FileChunkIO

import s3
import samples
import testsets.testcase as testcase
import utils
import s3_volumes
import file_generator
import file_ops


class TestMultipartUpload(testcase.FDSTestCase):
    
    '''
    * Try to multiupload a list of files sequentially first.
    * Then try to multiupload a list of files in parallel.
    '''
    
    def __init__(self, parameters=None, config_file=None,
                 om_ip_address=None):
        super(TestMultipartUpload, self).__init__(parameters=parameters,
                                                  config_file=config_file,
                                                  om_ip_address=om_ip_address)
        self.samples_size = []
        self.hash_table = {}
        self.s3_volume = s3_volumes.S3Volumes(self.om_ip_address)
        self.buckets = []
        self.f_ops = file_ops.FileOps(gb_size_files=True)
        
    def runTest(self):
        utils.create_dir(config.DOWNLOAD_DIR)
        # lets create 10 buckets, and try to upload the data files to them
        self.buckets = self.s3_volume.create_volumes(10, "multipart_upload")
        bucket_sample = random.choice(self.buckets)
        self.test_single_bucket_multipart_upload(bucket_sample)

    def test_single_bucket_multipart_upload(self, bucket):
        test_passed = False
        if bucket == None:
            raise ValueError("Bucket can't be None")
            self.reportTestCaseResult(test_passed)
        self.log.info("Testing multipart upload with bucket: %s" % bucket)
        files = self.f_ops.sample_files
        for f in files:
            base_dir = os.path.join("test_files", f)
            path = os.path.join(os.getcwd(), base_dir)
            self.s3_volume.store_multipart_uploads(bucket, path)
        self.log.info("Trying to Download the blobs now")
        #self.f_ops.download_files(bucket, config.DOWNLOAD_DIR)
        #self.f_ops.check_files_hash()
    
    def tearDown(self):
        pass
        #self.s3_volume.delete_volumes(self.buckets)
            
            
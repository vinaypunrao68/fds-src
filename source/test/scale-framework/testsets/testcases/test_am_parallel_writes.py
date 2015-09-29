#########################################################################
# @Copyright: 2015 by Formation Data Systems, Inc.                      #
# @file: test_am_parallel_writes.py                                                 #
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
import lib
import s3_volumes
import file_generator


class TestAMParallelWrites(testcase.FDSTestCase):
    
    '''
    * Perform the parallel writes to a single blob, then across many blobs
    at the same time.
    * Check for the system consistency.This test has a twin test, called
    TestAmParalleRead.
    '''
    
    def __init__(self, parameters=None, config_file=None,
                 om_ip_address=None):
        super(TestAMParallelWrites, self).__init__(parameters=parameters,
                                                   config_file=config_file,
                                                   om_ip_address=om_ip_address)

        self.test_connector = s3_volumes.S3Volumes(self.om_ip_address)
        # produce 1000 files of 256KB each, which will be used for the writes.
        self.fgen = file_generator.FileGenerator(1000, 256, 'K')
        self.buckets = []
        self.files = []
        
    def runTest(self):
        self.buckets = self.test_connector.create_volumes(250, "test_am_parallel")
        # generate the 1000 files.
        self.fgen.generate_files()
        self.files = self.fgen.get_files()
        files_sample = random.sample(self.files, 250)
        # pass a list of sample files, plus a random chosen bucket.
        test_passed = self.parallel_file_writes(files_sample,
                                                random.choice(self.buckets))
        buckets_sample = random.sample(self.buckets, 250)
        test_passed = self.parallel_bucket_writes(files_sample, buckets_sample)
        super(self.__class__, self).reportTestCaseResult(True)
    
    def parallel_file_writes(self, files, bucket):
        '''
        Write many files in parallel into the same bucket.
        '''
        rows = []
        for f in files:
            filepath, key_name = os.path.join(config.RANDOM_DATA, f), f
            rows.append((bucket, filepath, key_name))
        
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
                files_to_bucket = {executor.submit(self.test_connector.store_file_to_volume, 
                bucket, filepath, key_name): (bucket, filepath, key_name) for (bucket, filepath, key_name) in rows}
                for file in concurrent.futures.as_completed(files_to_bucket):
                    try:
                        pass
                    except Exception as exc:
                        self.log.exception('%r generated an exception: %s' % (url, exc))
                        return False
                    else:
                        self.log.info('%r file uploaded' % (file))
        return True
    
    def parallel_bucket_writes(self, files, buckets):
        '''
        Write a single file in parallel into different buckets
        '''
        rows = []
        for f, bucket in zip(files, buckets):
            filepath, key_name = os.path.join(config.RANDOM_DATA, f), f
            rows.append((bucket, filepath, key_name))
        
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            files_to_bucket = {executor.submit(self.test_connector.store_file_to_volume, 
            bucket, filepath, key_name): (bucket, filepath, key_name) for (bucket, filepath, key_name) in rows}
            for file in concurrent.futures.as_completed(files_to_bucket):
                try:
                    pass
                except Exception as exc:
                    self.log.exception('%r generated an exception: %s' % (url, exc))
                    return False
                else:
                    self.log.info('%r file uploaded' % (file))
        return True

    def tearDown(self):
        '''
        Delete all the buckets plus remove all the sample files created
        '''
        self.test_connector.delete_volumes(self.buckets)
        self.fgen.clean_up()
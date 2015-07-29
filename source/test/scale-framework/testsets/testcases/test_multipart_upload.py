#########################################################################
# @Copyright: 2015 by Formation Data Systems, Inc.                      #
# @file: test_multipart_upload.py                                                 #
# @author: Philippe Ribeiro                                             #
# @email: philippe@formationds.com                                      #
#########################################################################
import boto
import concurrent.futures
import contextlib
import glob
import hashlib
import math
import multiprocessing
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
from multiprocessing.pool import IMapIterator

import config
import config_parser
import file_generator
import file_ops
import s3
import s3_volumes
import samples
import testsets.testcase as testcase
import utils


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
        self.test_parallel_bucket_multipart_upload(bucket_sample)
        self.abort_multipart_upload(bucket_sample)
        self.test_multi_buckets_multipart_upload()
        self.reportTestCaseResult(True)

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
    
    def test_multi_buckets_multipart_upload(self):
        buckets = random.sample(self.buckets, 10)
        files = random.sample(self.f_ops.sample_files, 10)
        pairs = []
        # build a pair of bucket - files to be uploaded using multipart
        for bucket, f in zip(buckets, files):
            base_dir = os.path.join("test_files", f)
            path = os.path.join(os.getcwd(), base_dir)
            pairs.append((bucket, path))
        # do it in parallel
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            future_to_file = {executor.submit(self.s3_volume.store_multipart_uploads,
                                              bucket, path): (bucket, path) for bucket, path in pairs}
            for future in concurrent.futures.as_completed(future_to_file):
                file = future_to_file[future]
                try:
                    self.log.info(future.result())
                except Exception as exc:
                    self.log.exception('%s generated an exception: %s' % (file, exc))
                else:
                    self.log.info('file %s' % file)
        
    def test_parallel_bucket_multipart_upload(self, bucket):
        test_passes = False
        if bucket == None:
            raise ValueError("Bucket can't be None")
            self.reportTestCaseResult(test_passed)
        files = []
        for f in self.f_ops.sample_files:
             base_dir = os.path.join("test_files", f)
             path = os.path.join(os.getcwd(), base_dir)
             files.append(path)
        self.log.info("Testing multipart upload with bucket: %s" % bucket)
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            # start the load operations and mark each future with its files.
            future_to_file = {executor.submit(self.s3_volume.store_multipart_uploads,
                                              bucket, path): path for path in files}
            for future in concurrent.futures.as_completed(future_to_file):
                file = future_to_file[future]
                try:
                    self.log.info(future.result())
                except Exception as exc:
                    self.log.exception('%s generated an exception: %s' % (file, exc))
                else:
                    pass
    
    def abort_multipart_upload(self, bucket):
        try:
            test_passed = False
            if bucket == None:
                raise ValueError("Bucket can't be None")
                self.reportTestCaseResult(test_passed)
            self.log.info("Testing multipart upload with bucket: %s" % bucket)
            # just pick the first 5 files
            files = random.sample(self.f_ops.sample_files, 5)
            # pick a random sample to abort
            sample = random.choice(files)
            p = multiprocessing.Process(target=self.test_single_bucket_multipart_upload,
                                        name="Test Single Bucket", args=(bucket,))
            p.start()
            # wait 10 seconds for test_single_bucket_multipart_upload
            time.sleep(10)
            # terminate test_single_bucket_multipart_upload
            p.terminate()
            # cleanup
            p.join()
        except Exception, e:
            self.log.exception(e)
        finally:
            # try to upload the sample sample file that was interrupted.
            base_dir = os.path.join("test_files", sample)
            path = os.path.join(os.getcwd(), base_dir)
            self.s3_volume.store_multipart_uploads(bucket, path)
    
    
    def tearDown(self):
        self.s3_volume.delete_volumes(self.buckets)
            
            
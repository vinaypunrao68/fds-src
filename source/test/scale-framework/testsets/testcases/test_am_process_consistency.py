#########################################################################
# @Copyright: 2015 by Formation Data Systems, Inc.                      #
# @file: test_am_fio.py                                                 #
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
import fds_engine
import samples
import testsets.testcase as testcase
import utils

class TestAMProcessConsistency(testcase.FDSTestCase):
    
    '''
    Test works as following:
    * start the cluster
    * Add 50MB of data
    * shutdown the cluster
    * check for data consistency
    * repeat the process 3 times.
    
    The goal of the test is to ensure the AM processes work correctly when started
    and shutdown and data is preserved without loss or corruption.
    '''
    
    def __init__(self, parameters=None, config_file=None,
                 om_ip_address=None):
        super(TestAMProcessConsistency, self).__init__(parameters=parameters,
                                                       config_file=config_file,
                                                   om_ip_address=om_ip_address)

        self.ip_addresses = config_parser.get_ips_from_inventory(
                                                            self.inventory_file)
        # will have to change that so the inventory file is passed to the cluster
        self.engine = fds_engine.FdsEngine(self.inventory_file)
        self.sample_file = samples.sample_mb_files[len(samples.sample_mb_files) // 2]
        self.s3conn = s3.S3Connection(
                    config.FDS_DEFAULT_ADMIN_USER,
                    None,
                    self.om_ip_address,
                    config.FDS_S3_PORT,
                    self.om_ip_address,
                    )
        self.s3conn.s3_connect()

    @unittest.expectedFailure
    def runTest(self):
        # run the test 3 times
        for i in xrange(0, 3):
            self.test_volume_consistency()
            self.log.info("Sleeping for 30 seconds")
            time.sleep(30)
     
    def test_volume_consistency(self):   
        bucket_name = "volume_scale_test"
        bucket = self.s3conn.conn.create_bucket(bucket_name)
        if bucket == None:
            raise Exception("Invalid bucket.")
            # We won't be waiting for it to complete, short circuit it.
            self.test_passed = False
            self.reportTestCaseResult(self.test_passed)
            sys.exit(2)
        else:
            self.log.info("Bucket %s created" % bucket.name)
            self.store_file_to_volume(bucket)
            self.download_files(bucket)
            self.log.info("Shutting down all FDS processes")
            self.engine.stop_all_nodes_processes()
            self.log.info("Starting up all FDS processes")
            self.engine.start_all_nodes_processes()
            # will need to run a test for data consistency
    
    def download_files(self, bucket):
        '''
        Downloads all the files present in the bucket argument
        
        Arguments:
        ----------
        bucket : the S3 bucket object
        '''
        bucket_list = bucket.list()
        for l in bucket_list:
            key_string = str(l.key)
            path = os.path.join(config.DOWNLOAD_DIR, key_string)
            # check if file exists locally, if not: download it
            if not os.path.exists(path):
                self.log.info("Downloading %s" % path)
                l.get_contents_to_filename(path)
                
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
        path = os.path.join(config.TEST_DIR, self.sample_file)
        if os.path.exists(path):
            # produce a random generated number so the same file has different
            # names
            k.key = self.sample_file
            self.log.info("Uploading file %s" % k.key)
            k.set_contents_from_filename(path,
                                         cb=utils.percent_cb,
                                         num_cb=10)
            self.log.info("Uploaded file %s to bucket %s" % 
                         (self.sample_file, bucket.name))
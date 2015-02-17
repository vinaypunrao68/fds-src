import boto
import os
import random
import shutil
import sys
import time

from boto.s3.key import Key

import config
import s3
import users
import utils
import testsets.testcase as testcase

class TestGigabyteBlobSize(testcase.FDSTestCase):
    
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
        super(TestGigabyteBlobSize, self).__init__(parameters=parameters,
                                                    config_file=config_file,
                                                    om_ip_address=om_ip_address)
        
        self.sample_files = []
        self.hash_table = {}
    
    def runTest(self):
        if not os.path.exists(config.SAMPLE_DIR):
            os.makedirs(config.SAMPLE_DIR)
        if not os.path.exists(config.DOWNLOAD_DIR):
            os.makedirs(config.DOWNLOAD_DIR)

        s3conn = s3.S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            None,
            self.om_ip_address,
            config.FDS_S3_PORT,
        )
        s3conn.s3_connect()
        bucket_name = "volume_blob_test"
        bucket = s3conn.conn.create_bucket(bucket_name)
        if bucket == None:
            raise Exception("Invalid bucket.")
            # We won't be waiting for it to complete, short circuit it.
            self.test_passed = False
            self.reportTestCaseResult(self.test_passed)
        self.log.info("Volume %s created..." % bucket.name)
        # Store all the files created to the bucket
        self.create_random_size_files(bucket)
        self.log.info("Trying to Download the blobs now.")
        self.download_files(bucket)
        # Delete the bucket
        self.destroy_volume(bucket, s3conn)
        # remove the existing file
        if os.path.exists(config.SAMPLE_DIR):
            self.log.info("Removing %s" % config.SAMPLE_DIR)
            shutil.rmtree(config.SAMPLE_DIR)
        if os.path.exists(config.DOWNLOAD_DIR):
            self.log.info("Removing %s" % config.DOWNLOAD_DIR)
            shutil.rmtree(config.DOWNLOAD_DIR)
            
    def destroy_volume(self, bucket, s3conn):
        if bucket:
            for key in bucket.list():
                bucket.delete_key(key)
            self.log.info("Deleting bucket: %s", bucket.name)
            s3conn.conn.delete_bucket(bucket.name)
    
    def download_files(self, bucket):
        bucket_list = bucket.list()
        for l in bucket_list:
            key_string = str(l.key)
            path = os.path.join(config.DOWNLOAD_DIR, key_string)
            # check if file exists locally, if not: download it
            if not os.path.exists(path) and self.hash_table.has_key(key_string):
                self.log.info("Downloading %s" % path)
                start = time.time()
                l.get_contents_to_filename(path)
                end = time.time()
                elapsed = done - start
                self.log.info("Download time: %s" % round(elapsed, 2))
                hashcode = utils.hash_file_content(path)
                v = self.hash_table[key_string]
                if v != hashcode:
                    self.log.warning("%s != %s" % (v, hashcode))
                    self.test_passed = False
                    break
                # remove the file after it has been hashed.
                self.log.info("Removing file %s" % key_string)
                os.remove(path)
        self.test_passed = True

    def create_random_size_files(self, bucket):
        # lets produce 1000 files for each volume
        f_sample = "sample_file_%s"
        # We wanted to do a 10gb test ... but it won't very far!
        file_sizes = [1, 2, 3, 4, 5, 6, 7]
        for file_size in file_sizes:
            current = f_sample % file_size
            # produce a number between 1 - 1000
            if utils.create_file_sample(current, file_size, unit="G"):
                self.sample_files.append(current)
                path = os.path.join(config.SAMPLE_DIR, current)
                encode = utils.hash_file_content(path)
                self.hash_table[current] = encode
                
                path = os.path.join(config.SAMPLE_DIR, current)
                # Upload the file immediately, and remove its copy from the
                # repository
                if os.path.exists(path):
                    k = Key(bucket)
                    k.key = current
                    self.log.info("Uploading file %s ..." % current)
                    start = time.time()
                    k.set_contents_from_filename(path,
                                                 cb=utils.percent_cb,
                                                 num_cb=10)
                    end = time.time()
                    elapsed = end - start
                    self.log.info("Uploaded file %s to bucket %s" % 
                                 (current, bucket.name))
                    self.log.info("Upload time: %s" % round(elapsed, 2))
                    self.log.info("Removing file: %s" % current)
                    os.remove(path)
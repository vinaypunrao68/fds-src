import boto
import os
import random
import shutil
import sys
import unittest

from boto.s3.key import Key

import config
import s3
import samples
import users
import lib
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

    # @unittest.expectedFailure
    def runTest(self):

        lib.create_dir(config.DOWNLOAD_DIR)

        self.create_random_size_files()
        s3conn = s3.S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            None,
            self.om_ip_address,
            8000,
            self.om_ip_address,
            secure=False
        )
        s3conn.s3_connect()
        bucket_name = "volume_blob_test_mb"
        bucket = s3conn.conn.create_bucket(bucket_name)
        if bucket == None:
            raise Exception("Invalid bucket.")
            # We won't be waiting for it to complete, short circuit it.
            self.test_passed = False
            self.reportTestCaseResult(self.test_passed)
        self.log.info("Volume %s created..." % bucket.name)
        # Store all the files created to the bucket
        self.store_file_to_volume(bucket)
        self.log.info("Trying to Download the blobs now.")
        self.download_files(bucket)
        self.check_files_hash()
        # Delete the bucket
        self.destroy_volume(bucket, s3conn)
        # remove the existing file
        lib.remove_dir(config.DOWNLOAD_DIR)

    def check_files_hash(self):
        '''
        Assert the MD5 hash code of the files created and the files
        '''
        for k, v in self.hash_table.iteritems():
            # hash the current file, and compare with the key
            self.log.info("Hashing for file %s" % k)
            path = os.path.join(config.DOWNLOAD_DIR, k)
            hashcode = lib.hash_file_content(path)
            if v != hashcode:
                self.log.warning("%s != %s" % (v, hashcode))
                self.test_passed = False
                break
        self.test_passed = True

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
        #path = os.path.join(config.TEST_DIR, sample)
        for sample in self.sample_files:
            path = os.path.join(config.TEST_DIR, sample)
            if os.path.exists(path):
                k.key = sample
                k.set_contents_from_filename(path,
                                             cb=lib.percent_cb,
                                             num_cb=10)
                self.log.info("Uploaded file %s to bucket %s" %
                             (sample, bucket.name))

    def create_random_size_files(self):
        # lets upload the 1GB, 2GB and 3GB blocks.
        for current in samples.sample_gb_files:
            path = os.path.join(config.TEST_DIR, current)
            if os.path.exists(path):
                self.sample_files.append(current)
                encode = lib.hash_file_content(path)
                self.hash_table[current] = encode

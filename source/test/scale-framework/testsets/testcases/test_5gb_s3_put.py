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
import utils
import file_generator
import testsets.testcase as testcase

class Test5GBS3Put(testcase.FDSTestCase):
    '''
    Test whether the current XDI connector is able to perform a 5GB
    PUT request (upload). Check for data consistency.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(Test5GBS3Put, self).__init__(parameters=parameters,
                                           config_file=config_file,
                                           om_ip_address=om_ip_address)
        self.sample_files = []
        self.hash_table = {}
        self.fgen = file_generator.FileGenerator(5, 1, 'G')

    def runTest(self):
        # generate a 5GB file
        self.fgen.generate_files()
        self.sample_files = self.fgen.get_files()
        s3conn = s3.S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            None,
            self.om_ip_address,
            8000,
            self.om_ip_address,
            secure=False
        )
        s3conn.s3_connect()
        bucket_name = "5gb_block_test"
        bucket = s3conn.conn.create_bucket(bucket_name)
        if bucket == None:
            raise Exception("Invalid bucket.")
            self.test_passed = False
            self.reportTestCaseResult(self.test_passed)
        self.log.info("Volume %s created..." % bucket.name)
        # Store the 5GB file created in the bucket
        self.store_file_to_volume(bucket)
        self.log.info("Trying to Download the blobs now.")
        self.download_files(bucket)
        self.check_file_hash()
        # Delete the bucket
        self.destroy_volume(bucket, s3conn)
        # remote the 5GB file created
        utils.remove_dir(config.RANDOM_DATA)

    def check_file_hash(self):
        '''
        Assert the MD5 hash code of the file creates and the file uplaoded
        are the same.
        '''
        for k, v in self.hash_table.iteritems():
            # hash the file and compare with the key
            self.log.info("Hashing for file %s" % k)
            path = os.path.join(config.RANDOM_DATA, k)
            hashcode = utils.hash_file_content(path)
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
            path = os.path.join(config.RANDOM_DATA, key_string)
            # check if file exists locally, if not: download it
            if not os.path.exists(path):
                self.log.info("Downloading %s" % path)
                l.get_contents_to_filename(path)

    def store_file_to_volume(self, bucket):
        k = Key(bucket)
        for sample in self.sample_files:
            path = os.path.join(config.RANDOM_DATA, sample)
            if os.path.exists(path):
                k.key = sample
                k.set_contents_from_filename(path,
                                             cb=utils.percent_cb,
                                             num_cb=10)
                self.log.info("Uploaded file %s to bucket %s" %
                             (sample, bucket.name))

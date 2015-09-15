################################################################################
#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
# author: Philippe Ribeiro
# email: philippe@formationds.com
# file: test_100gb_volume.py
###############################################################################
import lib
import config
import unittest
import requests
import json
import random
import sys
import time
import shutil

import ssh
import os
import lib

nbd_path = os.path.abspath(os.path.join('..', ''))
sys.path.append(nbd_path)

from boto.s3.key import Key

import testsets.testcase as testcase
#import testlib.SM as sm
#import testlib.DM as dm
import lib
import file_generator
import s3_volumes

class TestIOLoadConsistency(testcase.FDSTestCase):
    '''
    While under load with I/O to the same and different volumes/blobs from
    several applications, shoot the AM and verify that DM/SM groups are at least
    prefix consistent.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestIOLoadConsistency, self).__init__(parameters=parameters,
                                               config_file=config_file,
                                               om_ip_address=om_ip_address)

        self.fgen = file_generator.FileGenerator(1, 1, 'G')
        self.test_connector = s3_volumes.S3Volumes(self.om_ip_address)
        self.buckets = []
        self.files = []

    def runTest(self):
        self.buckets = self.test_connector.create_volumes(10, "test_io_load_consistency")
        self.fgen.generate_files()
        self.files = self.fgen.get_files()
        f = random.choice(self.files)
        bucket = random.choice(self.buckets)
        filepath, key_name = os.path.join(config.RANDOM_DATA, f), f
        self.test_connector.store_file_to_volume(bucket, filepath, key_name)
        super(self.__class__, self).reportTestCaseResult(True)
    
    def tearDown(self):
        '''
        Delete all the buckets plus remove all the sample files created
        '''
        self.test_connector.delete_volumes(self.buckets)
        self.fgen.clean_up()
    
class TestIOReadConsistency(testcase.FDSTestCase):
    '''
    Following a load of I/O to the same and different volumes/blobs from several
    applications, stop the I/O and verify that DM/SM groups eventually become
    immediately consistent.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestIOReadConsistency, self).__init__(parameters=parameters,
                                               config_file=config_file,
                                               om_ip_address=om_ip_address)

        self.fgen = file_generator.FileGenerator(1, 1, 'G')
        self.test_connector = s3_volumes.S3Volumes(self.om_ip_address)
        self.buckets = []
        self.files = []

    def runTest(self):
        self.buckets = self.test_connector.create_volumes(10, "test_io_read_consistency")
        self.fgen.generate_files()
        self.files = self.fgen.get_files()
        f = random.choice(self.files)
        bucket = random.choice(self.buckets)
        filepath, key_name = os.path.join(config.RANDOM_DATA, f), f
        self.test_connector.store_file_to_volume(bucket, filepath, key_name)
        super(self.__class__, self).reportTestCaseResult(True)
    
    def tearDown(self):
        '''
        Delete all the buckets plus remove all the sample files created
        '''
        self.test_connector.delete_volumes(self.buckets)
        self.fgen.clean_up()
    
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

import config
import s3
import samples
import testsets.testcase as testcase
import lib

class TestMultiConnToMultiVolume(testcase.FDSTestCase):
    
    id_number = 1
    def __init__(self, parameters=None, config_file=None,
                om_ip_address=None):
        super(TestMultiConnToMultiVolume, self).__init__(
                                                    parameters=parameters,
                                                    config_file=config_file,
                                                    om_ip_address=om_ip_address)
        
        self.ip_addresses = config_parser.get_ips_from_inventory(
                                           self.inventory_file)
        self.hash_table = {}
        self.s3_connections_table = {}
        self.sample_files = []
        self.volumes_count = config.MAX_NUM_VOLUMES
        # lets start with 10 connections for now ..
        lib.create_dir(config.TEST_DIR)
        lib.create_dir(config.DOWNLOAD_DIR)
            
        # for this test, we will create 5 sample files, 2MB each
        for current in samples.sample_mb_files[:3]:
            path = os.path.join(config.TEST_DIR, current)
            if os.path.exists(path):
                self.sample_files.append(current)
                encode = lib.hash_file_content(path)
                self.hash_table[current] = encode
        self.log.info("hash table: %s" % self.hash_table)   

    def create_s3_connection(self, ip):
        '''
        Create a single s3 connection, given the host ip address.
        
        Attributes:
        -----------
            ip : str
            the host's ip address
        Returns:
        --------
            s3conn - a S3 connection object
        '''
        # creates 20 s3 connections for each AM node
        s3conn = s3.S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            None,
            ip,
            config.FDS_S3_PORT,
            self.om_ip_address,
        )
        s3conn.s3_connect()
        return s3conn

    @unittest.expectedFailure
    def runTest(self):
        self.log.info("Starting the multi connection to multi volume test...\n")
        bucket_name = "volume0%s-test"
        for ip in self.ip_addresses:
            for i in xrange(0, 10):
                s3conn = self.create_s3_connection(ip)
                bucket = self.create_volume(s3conn, (bucket_name % self.id_number))
                self.s3_connections_table[s3conn] = bucket
                # sleep for 3 seconds
                time.sleep(3)
                self.id_number += 1

        self.concurrently_volumes()
        self.log.info("Removing the sample files created.")
        lib.remove_dir(config.DOWNLOAD_DIR)
        self.reportTestCaseResult(self.test_passed)
    
    def concurrently_volumes(self):
        '''
        Execute a task in parallel, (in this case, store_file_to_volume), 
        using a Thread pool. The goal is to run tests in parallel, given the
        map between connections and S3 volumes.
        '''
        self.log.info("Starting multithreading store files")
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            future_volumes = { executor.submit(self.store_file_to_volume, bucket): 
                                 bucket for s3conn, bucket in \
                                 self.s3_connections_table.iteritems() }
            for future in concurrent.futures.as_completed(future_volumes):
                try:
                    # self.delete_volume(s3conn, bucket)
                    self.test_passed = True
                except Exception as exc:
                    self.log.exception('generated an exception: %s' % exc)
                    self.test_passed = False
                    break
        # clean up existing buckets
        for s3conn, bucket in self.s3_connections_table.iteritems():
            self.delete_volume(s3conn, bucket)
        self.reportTestCaseResult(self.test_passed)   

    @unittest.expectedFailure
    def run_tasks(self, s3conn, bucket):
        # 1) Store files to the volumes
        self.store_file_to_volume(bucket)
        # 2) Download contents and hash values
        self.download_files(bucket)
        # 3) Compare contents
        self.validade_data()
        # 4) Delete buckets
        self.delete_volume(s3conn, bucket)
    
    def create_volume(self, s3conn, bucket_name):
        '''
        For each of the AM instances, create a single bucket for every volume,
        and populate that volume with sample data.
        
        Attributes:
        -----------
        s3conn: S3Connection
            a S3Connection to the host machine
        bucket_name:
            the name of the bucket being created
            
        Returns:
        --------
        bucket : The S3 bucket object
        '''
        bucket = s3conn.conn.create_bucket(bucket_name)
        if bucket == None:
            raise Exception("Invalid bucket.")
            # We won't be waiting for it to complete, short circuit it.
            self.test_passed = False
            self.reportTestCaseResult(self.test_passed)

        self.log.info("Volume %s created..." % bucket.name)
        return bucket
    
    def validade_data(self):
        '''
        Given a hash value between the file contents and its name,
        assert that the file uploaded and the file downloaded have the
        same hash key.
        '''
        for k, v in self.hash_table.iteritems():
            # hash the current file, and compare with the key
            self.log.info("Hashing for file %s" % k)
            path = os.path.join(config.DOWNLOAD_DIR, k)
            hashcode = lib.hash_file_content(path)
            if v != hashcode:
                self.log.warning("%s != %s" % (v, hashcode))
                self.test_passed = False
                self.reportTestCaseResult(self.test_passed)
        self.test_passed = True

    def download_files(self, bucket):
        '''
        Download all the files present in a S3 volume
        
        Attributes:
        -----------
        bucket: The s3 volume (bucket) to where download the data from
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
        for sample in self.sample_files:
            path = os.path.join(config.TEST_DIR, sample)
            if os.path.exists(path):
                k.key = sample
                k.set_contents_from_filename(path,
                                             cb=lib.percent_cb,
                                             num_cb=10)
                self.log.info("Uploaded file %s to bucket %s" % 
                             (sample, bucket.name))

    def delete_volume(self, s3conn, bucket):
        '''
        After test execute, remove the current bucket associated with this
        connection
        
        Attributes:
        -----------
        s3conn : S3Conn
            the connection object to the FDS instance via S3
        bucket: S3
            the bucket associated with this connection
        '''
        if bucket:
            for key in bucket.list():
                bucket.delete_key(key)
            self.log.info("Deleting bucket: %s", bucket.name)
            s3conn.conn.delete_bucket(bucket.name)
            s3conn.s3_disconnect()

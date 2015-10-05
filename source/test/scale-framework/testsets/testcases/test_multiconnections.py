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
import samples
import testsets.testcase as testcase
import lib

class TestMultiConnections(testcase.FDSTestCase):
    
    def __init__(self, parameters=None, config_file=None,
                om_ip_address=None):
        super(TestMultiConnections, self).__init__(parameters=parameters,
                                                   config_file=config_file,
                                                   om_ip_address=om_ip_address)

        if self.inventory_file is None:
            self.log.exception("Inventory file can't be null for this test")
            raise ValueError, "Inventory file can't be null for this test"
            sys.exit(2)

        self.ip_addresses = config_parser.get_ips_from_inventory(
                                           self.inventory_file)
        self.buckets = []
        self.hash_table = {}
        self.sample_files = []
        self.s3_connections = []
    
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

    def create_multiple_connections(self, ip):
        # creates 20 s3 connections for each AM node
        for i in xrange(0, 10):
            s3conn = s3.S3Connection(
                config.FDS_DEFAULT_ADMIN_USER,
                None,
                ip,
                config.FDS_S3_PORT,
                self.om_ip_address,
            )
            s3conn.s3_connect()
            self.s3_connections.append(s3conn)

    def runTest(self):
        self.log.info("Starting the multivolume test...\n")
        for ip in self.ip_addresses[:1]:
            # create multiple connections
            #self.log.info("Processing IP: %s" % ip)
            self.create_multiple_connections(ip)
            self.concurrently_volumes()
        self.log.info("Removing the sample files created.")
        lib.remove_dir(config.DOWNLOAD_DIR)
        self.reportTestCaseResult(self.test_passed)
    
    def concurrently_volumes(self):
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            future_volumes = { executor.submit(self.create_volumes, s3conn): 
                                 s3conn for s3conn in self.s3_connections}
            for future in concurrent.futures.as_completed(future_volumes):
                # s3conn = future_volumes[future]
                try:
                    # self.delete_volumes(s3conn)
                    self.test_passed = True
                except Exception as exc:
                    self.log.exception('generated an exception: %s' % (exc))
                    self.test_passed = False
                        
        
    def create_volumes(self, s3conn):
        '''
        For each of the AM instances, establish a new connection and create
        the MAX_NUM_VOLUMES supported. 
        
        Attributes:
        -----------
        ip: str
            the ip address of that AM node.
            
        Returns:
        --------
        '''
        bucket_name = "volume0%s-test"
        buckets = []
        for i in xrange(0, 1):
            bucket = s3conn.conn.create_bucket(bucket_name % i)
            if bucket == None:
                raise Exception("Invalid bucket.")
                # We won't be waiting for it to complete, short circuit it.
                self.test_passed = False
                self.reportTestCaseResult(self.test_passed)
                sys.exit(2)

            self.log.info("Bucket %s created" % bucket.name)
            self.buckets.append(bucket)
            self.store_file_to_volume(bucket)
            
            # After creating volume and uploading the sample files to it
            # ensure data consistency by hashing (MD5) the file and comparing
            # with the one stored in S3
            self.download_files(bucket)
            for k, v in self.hash_table.iteritems():
                # hash the current file, and compare with the key
                self.log.info("Hashing for file %s" % k)
                path = os.path.join(config.DOWNLOAD_DIR, k)
                hashcode = lib.hash_file_content(path)
                if hashcode == None:
                    continue
                elif v != hashcode:
                    self.log.exception("%s != %s" % (v, hashcode))
                    self.test_passed = False
                    self.reportTestCaseResult(self.test_passed)
                    break
                else:
                    self.test_passed = True
        return s3conn

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
        #path = os.path.join(config.SAMPLE_DIR, sample)
        for sample in self.sample_files:
            path = os.path.join(config.TEST_DIR, sample)
            if os.path.exists(path):
                k.key = sample
                k.set_contents_from_filename(path,
                                             cb=lib.percent_cb,
                                             num_cb=10)
                self.log.info("Uploaded file %s to bucket %s" % 
                             (sample, bucket.name))

    def delete_volumes(self, s3conn):
        '''
        After test executes, remove existing buckets.
        
        Attributes:
        -----------
        s3conn : S3Conn
            the connection object to the FDS instance via S3
            
        '''
        try:
            for bucket in self.buckets:
                if bucket:
                    s3.meta.client.head_bucket(Bucket=bucket.name)
                    for key in bucket.list():
                        bucket.delete_key(key)
                    self.log.info("Deleting bucket: %s", bucket.name)
                    s3conn.conn.delete_bucket(bucket.name)
            self.buckets = []
        except Exception:
            return
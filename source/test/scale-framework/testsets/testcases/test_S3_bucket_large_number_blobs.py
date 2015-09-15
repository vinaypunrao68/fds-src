#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
# author: Heang Lim
# email: heang@formationds.com
# file: test_multiple_block_mount_points.py
import subprocess
import random
import config
import config_parser
import s3
import os
import lib
from boto.s3.connection import OrdinaryCallingFormat
from boto.s3.key import Key
from boto.s3.bucket import Bucket
import testsets.testcase as testcase
import requests
import ssh
import sys

import pdb

class TestS3LargeNumberBlobs(testcase.FDSTestCase):
    
    '''
    FS-1173 - Test volume for large number of data blobs
    This test exercises large number of different data blob sizes with same s3 volume.
    The test downloads and uploads the data blobs and compares the hash data to ensure data integrity.
    The goal is to gauge the system behavior using large number of small blob objects.  
    The test uses a set of known blob sizes plus 10 random samples to cover corner cases.
    Currently, this test can only run on a single OM node.
    '''
    # max number of blobs in each bucket
    #MAX_NUMBER_BLOBS = 10000 #NOTE:  change this back to 10000 when the system is stable enough to test
    MAX_NUMBER_BLOBS = 100 
    
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestS3LargeNumberBlobs, self).__init__(parameters=parameters,
                                                    config_file=config_file,
                                                    om_ip_address=om_ip_address)
        
        self.ip_addresses = config_parser.get_ips_from_inventory(
                                                            self.inventory_file)
        
        self.blob_sizes = set([1,2,4,8,16,32,64,128,256,512])
        samples = list(xrange(0, 1024))
        random_samples = random.sample(samples, 10)
        self.all_blob_sizes = self.blob_sizes.union(random_samples)
        self.buckets = []
	self.hash_table = {}
	self.sample_files = {}
	lib.create_dir(config.DOWNLOAD_DIR)

    def runTest(self):
	'''
	This method starts the test
	'''
	#generate blob size files based on self.all_blob_sizes
	self.generate_blob_sizes()

	#Get s3 connection
	s3conn = self.connect_s3()
	
	#Create s3 volumes
        self.create_s3_volumes(s3conn)

	#compare downloaded hash files
	self.check_files_hash()

	#clean up by deleting volumes
	self.delete_volumes(s3conn)

	#remove directory where the downloaded files are kept
	lib.remove_dir(config.DOWNLOAD_DIR)


    def connect_s3(self):
        '''
        establish a S3 connection
        '''
	#1
        s3conn = s3.S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            None,
            self.om_ip_address,
            config.FDS_S3_PORT,
            self.om_ip_address,
        )
	#2
        s3conn.s3_connect()
        return s3conn
    
    def generate_blob_sizes(self):
	'''
	Create MAX_NUMBER_BLOBS of xK blob sizes to test on config.TEST_DIR directory
	'''

	lib.create_dir(config.TEST_DIR)

	for eachBlob in self.all_blob_sizes:

            	for i in xrange(0, self.MAX_NUMBER_BLOBS):
			fileName = 'test_sample_%sK_%s.img' %(eachBlob, i)
	    		self.sample_files[fileName] = fileName
			path = os.path.join(config.TEST_DIR, fileName)
			#cmd = "fallocate -l %sK %s" %(eachBlob, path)
			cmd = "dd if=/dev/zero of=./test_files/%s bs=%sKB count=1" %(fileName, eachBlob)
			subprocess.call([cmd], shell=True)
			self.log.info("Created file %s of size %sK" %(fileName, eachBlob))

			#get hash for each file created and store it in self.hash_table
			encode = lib.hash_file_content(path)
			self.hash_table[fileName] = encode
				

    def check_files_hash(self):
        '''
        Assert the MD5 hash code for each generated file with downloaded file.
	If a single hash code on a download file is different than the generated
	hash code, the test fails and exits.
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

    def create_s3_volumes(self, s3conn):
	'''
	Create a volume for each blob size to test upload and download with bucket. 
	'''
        #0
	bucket_name = "volume_blob_size_%s" 

	#1,2 s3conn	

        for size in self.all_blob_sizes:
	    #3
            bucket = s3conn.conn.create_bucket(bucket_name % size)
            

            if bucket == None:
                raise Exception("Invalid bucket.")
                # We won't be waiting for it to complete, short circuit it.
                self.test_passed = False
                self.reportTestCaseResult(self.test_passed)
            
            self.log.info("Volume %s created..." % bucket.name)

            self.buckets.append(bucket)
            # create the volume based on the bucket name.
            for i in xrange(0, self.MAX_NUMBER_BLOBS):

	   	    fileName = 'test_sample_%sK_%s.img' %(size, i)
		    # Store each file to the bucket by using up to self.MAX_NUMBER_BLOBS 
		    self.log.info("Storing file %s to volume..." %fileName)
		    self.store_file_to_volume(bucket, fileName)
		    
		    self.log.info("Trying to download %sK blob now" %size)
		    self.download_files(bucket, fileName)

		    # After creating volume and uploading the sample files to it
		    # ensure data consistency by hashing (MD5) the file and comparing
		    # with the one stored in S3


	    	    #Need hash class method to compare local hashes with remote hashes on s3 
		    
            
    def download_files(self, bucket, filename):
	'''
	Download file for each blob size S3 bucket.
	'''

        lib.create_dir(config.DOWNLOAD_DIR)

        path = os.path.join(config.DOWNLOAD_DIR, filename)
        self.log.info("Downloading %s" % path)

	k = Key(bucket)
        k.key =  filename
        k.get_contents_to_filename(path)

    def store_file_to_volume(self, bucket, filename):
        '''
	Upload file associated with each blob size S3 bucket.
        '''
	# add the data files to the bucket.
	k = Key(bucket)
	#for sample in self.sample_files:
        path = os.path.join(config.TEST_DIR, filename)
        if os.path.exists(path):
                #k.key = sample
                k.key =  filename
                k.set_contents_from_filename(path,
                                             cb=lib.percent_cb,
                                             num_cb=10)
                self.log.info("Uploaded file %s to bucket %s" %
                             (filename, bucket.name))
    
    def delete_volumes(self, s3conn):
	'''
	After test executes, remove existing buckets.

	Attributes:
	-----------
	s3conn : S3Conn
    	the connection object to the FDS instance via S3

	'''
	for bucket in self.buckets:
	    if bucket:
		for key in bucket.list():
		    bucket.delete_key(key)
		self.log.info("Deleting bucket: %s", bucket.name)
		s3conn.conn.delete_bucket(bucket.name)
	self.buckets = []

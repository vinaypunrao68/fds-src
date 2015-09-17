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
import file_generator
import shutil
import unittest

import pdb

class TestS3BucketLargeNumberBlobsForCornerCases(testcase.FDSTestCase):
    
    '''
    FS-1173 - Test volume for large number of data blobs
    This test exercises large number of different 0bytes data blob size with same s3 volume.
    The test downloads and uploads the data blobs and compares the hash data to ensure data integrity.
    The goal is to gauge the system behavior using large number of 0bytes blob objects.  
    Currently, this test can only run on a single OM node.
    '''
    # max number of blobs in each bucket
    MAX_NUMBER_BLOBS = 500
    NUMBER_OF_ITERATION = 2 	#run test this number of times
    
    
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestS3BucketLargeNumberBlobsForCornerCases, self).__init__(parameters=parameters,
                                                    config_file=config_file,
                                                    om_ip_address=om_ip_address)
        
        self.ip_addresses = config_parser.get_ips_from_inventory(
                                                            self.inventory_file)
        
        self.all_blob_sizes = [0]
        self.buckets = []
	self.hash_table_download = {}
	self.hash_table = {}
	self.sample_files = {}
	lib.create_dir(config.DOWNLOAD_DIR)

    @unittest.expectedFailure
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

	#compare downloaded hash files with generated hash files
	self.check_files_hash()

	#clean up by deleting volumes
	self.delete_volumes(s3conn)

	#remove directory where the downloaded files are kept
	lib.remove_dir(config.DOWNLOAD_DIR)

	self.clean_up_generated_files()

    def connect_s3(self):
        '''
        establish a S3 connection
        '''
        s3conn = s3.S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            None,
            self.om_ip_address,
            config.FDS_S3_PORT,
            self.om_ip_address,
        )
        s3conn.s3_connect()
        return s3conn
    
    def generate_blob_sizes(self):
	'''
	Create MAX_NUMBER_BLOBS of 0bytes blob size files to test on config.TEST_DIR directory
	'''

	#Use file_generator to create files to test
	for eachBlob in self.all_blob_sizes:
		fgen = file_generator.FileGenerator(eachBlob, self.MAX_NUMBER_BLOBS, 'K')
		fgen.generate_files()


	#get hashes for each generated file
	#NOTE:  hash_file_content does not return hash with 0 byptes file
        for i in xrange(0, self.MAX_NUMBER_BLOBS):
		fileName = 'file_%s_%sK' %(i, eachBlob)
	    	#	self.sample_files[fileName] = fileName
		path = os.path.join(config.RANDOM_DATA, fileName)
		#	#get hash for each file created and store it in self.hash_table
		cmd = 'md5sum %s' %(path)
		p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
		out,err = p.communicate()
		encode = out.split()
		self.hash_table[fileName] = encode[0]
		

    def check_files_hash(self):
        '''
        Assert the MD5 hash code for each generated file with downloaded file.
	If a single hash code on a download file is different than the generated
	hash code, the test fails.
        '''

	if not lib.compare_hashes(self.hash_table, self.hash_table_download):
		self.test_passed = False

	else:
        	self.test_passed = True

    def create_s3_volumes(self, s3conn):
	'''
	Create a volume for each blob size to test upload and download with bucket. 
	'''
	bucket_name = "volume_blob_size_%s" 


        for size in self.all_blob_sizes:
		bucket = s3conn.conn.create_bucket(bucket_name % size)

		if bucket == None:
			raise Exception("Invalid bucket.")
			# We won't be waiting for it to complete, short circuit it.
			self.test_passed = False
			self.reportTestCaseResult(self.test_passed)


		else:
			self.log.info("Creating volume %s ..." % bucket.name)

			self.buckets.append(bucket)
			# create the volume based on the bucket name.
			for i in xrange(0, self.MAX_NUMBER_BLOBS):

				fileName = 'file_%s_%sK' %(i, size)
				path=os.path.join(config.DOWNLOAD_DIR, fileName)
				# Store each file to the bucket by using up to self.MAX_NUMBER_BLOBS 
				self.log.info("Storing file %s to volume..." %fileName)
				self.store_file_to_volume(bucket, fileName)

				self.log.info("Trying to download %s blob now" %fileName)
				self.download_files(bucket, fileName)

				#get hash for each downloaded file and store it in self.hash_table
				cmd = 'md5sum %s' %(path)
				p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
				out,err = p.communicate()
				encode = out.split()
				self.hash_table_download[fileName] = encode[0]



    
    def download_files(self, bucket, filename):
	'''
	Download files for each blob size to S3 bucket.
	'''

        lib.create_dir(config.DOWNLOAD_DIR)

        path = os.path.join(config.DOWNLOAD_DIR, filename)
        self.log.info("Downloading %s" % path)

	k = Key(bucket)
        k.key =  filename
        k.get_contents_to_filename(path)

    def store_file_to_volume(self, bucket, filename):
        '''
	Upload files associated with each blob size to S3 bucket.
        '''
	# add the data files to the bucket.
	k = Key(bucket)
	#for sample in self.sample_files:
        path = os.path.join(config.RANDOM_DATA, filename)
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

    def clean_up_generated_files(self):
	'''
	remove the files generated for testing
	'''
	
	shutil.rmtree(config.RANDOM_DATA)	



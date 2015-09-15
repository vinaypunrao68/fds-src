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
import json
import block_volumes
import file_generator

import pdb

class TestBlockVolumeLargeNumberBlobsForCornerCases(testcase.FDSTestCase):
    
    '''
    FS-1325- Test volume for large number of data blobs for corner cases
    This test exercises large number of different data blob sizes with block/volume for 0KB files.
    The test downloads and uploads the data blobs and compares the hash data to ensure data integrity.
    The goal is to gauge the system behavior using large number of small blob objects.  
    Currently, this test can only run on a single OM node.
    '''
    # max number of blobs in block volume
    MAX_NUMBER_BLOBS =  100 #create number of files to test
    NUMBER_OF_ITERATION = 1  #number of times to run the test.  NOTE: this could take a long time for mkfs.ext2

    
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestBlockVolumeLargeNumberBlobsForCornerCases, self).__init__(parameters=parameters,
                                                    config_file=config_file,
                                                    om_ip_address=om_ip_address)
        
        self.ip_addresses = config_parser.get_ips_from_inventory(
                                                            self.inventory_file)
        
        self.all_blob_sizes = [0] 
        self.filesystem_types = ['mkfs.ext2', 'mkfs.ext3', 'mkfs.ext4'] #not sure if we need ext2
        #self.filesystem_types = ['mkfs.ext4'] #not sure if we need ext2
        lib.create_dir(config.DOWNLOAD_DIR)

	self.bv = block_volumes.BlockVolumes(self.om_ip_address)

    def runTest(self):
	'''
	This method starts the test
	'''
	#run the test NUMBER_OF_ITERATION times
        for i in range(self.NUMBER_OF_ITERATION):
               
        	for filesystem_type in self.filesystem_types:
        		self.hash_table = {}
        		self.hash_table_fdsmount = {}
        		self.sample_files = {}
        		self.local_ssh = ssh.SSHConn(config.NDBADM_CLIENT, config.SSH_USER, config.SSH_PASSWORD)
        		#generate blob size files based on self.all_blob_sizes
        		self.generate_blob_sizes()
        
        		#Create block volume
        		self.Create_Block_Volumes(filesystem_type)



    def generate_blob_sizes(self):
	'''
	Create MAX_NUMBER_BLOBS of xK blob sizes to test on nbd client
	'''

        for eachBlob in self.all_blob_sizes:

             for i in xrange(0, self.MAX_NUMBER_BLOBS):
                 fileName = 'file_%s_%sK' %(i, eachBlob)
                 self.sample_files[fileName] = fileName
		 #creating 0bytes files on the nbd-client
                 cmd = "touch %s" %(fileName)
                 self.log.info("Creating file %s of size %sK" %(fileName, eachBlob))
                 (stdin, stdout, stderr) = self.local_ssh.client.exec_command(cmd)
                 self.log.info(stdout.readlines())		

		 #get hash for each file created and store it in self.hash_table
                 (stdin, stdout, stderr) = self.local_ssh.client.exec_command('md5sum %s' %fileName)
                 for line in stdout:
		 	encode = line.split()
		 	self.hash_table[fileName] = encode[0]
				

    def Create_Block_Volumes(self, filesystem_type):
	'''
	Create a block volume for to test with blob size files. 
	'''
        #0
        volume_name = "block_volume_ts1325" 
		    
        test_passed = False
        r = None
        port = config.FDS_REST_PORT
        try:

		self.bv.create_volumes(1, volume_name, "100", 'GB')
		volume_name = "block_volume_ts1325_0"  #API appends _0 to volume name 

            	#create mount point
		cmd = 'mkdir /fdsmount'
		(stdin, stdout, stderr) = self.local_ssh.client.exec_command(cmd)
                self.log.info(stdout.readlines())

		#attach volume to device
		cmd = './nbdadm.py attach %s %s' %(self.om_ip_address, volume_name)
		(stdin, stdout, stderr) = self.local_ssh.client.exec_command(cmd)
                for line in stdout:
			deviceName = line.strip()
			selectedDevice = json.dumps(deviceName)[1:-1]
			
	        cmds = (
			'%s %s' %(filesystem_type, selectedDevice),
			'mount %s /fdsmount' %(selectedDevice)
			)

		#mount device
		for cmd in cmds:
			self.log.info("Executing %s" %cmd)
			(stdin, stdout, stderr) = self.local_ssh.client.exec_command(cmd)
			self.log.info(stdout.readlines())

		#move small blobs of files to block mount point
		for eachfile in self.sample_files:
			cmd = 'mv %s /fdsmount' %eachfile
			self.log.info("Executing %s" %cmd)
			(stdin, stdout, stderr) = self.local_ssh.client.exec_command(cmd)
                	self.log.info(stdout.readlines())

			(stdin, stdout, stderr) = self.local_ssh.client.exec_command('md5sum /fdsmount/%s' %eachfile)

			#capture only hash value in stdout from /fdsmount/<filename>
			for line in stdout:
				encode = line.split()
				self.hash_table_fdsmount[eachfile] = encode[0]
			
		

		#clean up
		cmds = (
				'umount /fdsmount',
				'rm -rf /fdsmount',
				'./nbdadm.py detach %s' %(volume_name)
			)

		for cmd in cmds:
			self.log.info("Executing %s" %cmd)
			(stdin, stdout, stderr) = self.local_ssh.client.exec_command(cmd)
			self.log.info(stdout.readlines())
				

		#compare hashes for small blob of files moved to /fdsmount with local file hashes
		#If hashes don't match for even a single file, test fails
		if lib.compare_hashes(self.hash_table, self.hash_table_fdsmount):
			test_passed=True

		else:
			test_passed=False
		
		if self.bv.delete_volume(volume_name) == True:
			self.log.info('Volume %s has been deleted' %volume_name)

		else:
			self.log.info('NOTE:  Volume %s has not been deleted' %volume_name)

        except Exception, e:
            self.log.exception(e)

        finally:
            self.local_ssh.client.close()
            super(self.__class__, self).reportTestCaseResult(test_passed)


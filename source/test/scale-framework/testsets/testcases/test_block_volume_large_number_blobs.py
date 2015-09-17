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

import pdb

class TestBlockVolumeLargeNumberBlobs(testcase.FDSTestCase):
    
    '''
    FS-1173 - Test volume for large number of data blobs
    This test exercises large number of different data blob sizes with block/volume.
    The test downloads and uploads the data blobs and compares the hash data to ensure data integrity.
    The goal is to gauge the system behavior using large number of small blob objects.  
    The test uses a set of known blob sizes plus 10 random samples to cover corner cases.
    Currently, this test can only run on a single OM node.
    '''
    # max number of blobs in block volume
    #MAX_NUMBER_BLOBS = 10000
    MAX_NUMBER_BLOBS = 100 #NOTE:  test can take a long time to complete if this value > 100
    
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestBlockVolumeLargeNumberBlobs, self).__init__(parameters=parameters,
                                                    config_file=config_file,
                                                    om_ip_address=om_ip_address)
        
        self.ip_addresses = config_parser.get_ips_from_inventory(
                                                            self.inventory_file)
        
        self.blob_sizes = set([1,2,4,8,16,32,64,128,256,512])
        samples = list(xrange(0, 1024))
        random_samples = random.sample(samples, 10)
        self.all_blob_sizes = self.blob_sizes.union(random_samples)
	self.filesystem_types = ['mkfs.ext2', 'mkfs.ext4', 'mkfs.ext3'] #not sure if we need ext2
	lib.create_dir(config.DOWNLOAD_DIR)

    def runTest(self):
	'''
	This method starts the test
	'''
	for filesystem_type in self.filesystem_types:
		self.hash_table = {}
		self.hash_table_fdsmount = {}
		self.sample_files = {}
		self.local_ssh = ssh.SSHConn(config.NDBADM_CLIENT, config.SSH_USER, config.SSH_PASSWORD)
		#generate blob size files based on self.all_blob_sizes
		self.generate_blob_sizes()

		#Create block volume
		self.create_block_volumes(filesystem_type)



    def generate_blob_sizes(self):
	'''
	Create MAX_NUMBER_BLOBS of xK blob sizes to test on nbd client
	'''

	for eachBlob in self.all_blob_sizes:

            	for i in xrange(0, self.MAX_NUMBER_BLOBS):
			fileName = 'test_sample_%sK_%s.img' %(eachBlob, i)
	    		self.sample_files[fileName] = fileName
			cmd = "dd if=/dev/zero of=%s bs=%sKB count=1" %(fileName, eachBlob)
			self.log.info("Creating file %s of size %sK" %(fileName, eachBlob))
			(stdin, stdout, stderr) = self.local_ssh.client.exec_command(cmd)
			self.log.info(stdout.readlines())		

			#get hash for each file created and store it in self.hash_table
			(stdin, stdout, stderr) = self.local_ssh.client.exec_command('md5sum %s' %fileName)
			for line in stdout:
				encode = line.split()
				self.hash_table[fileName] = encode[0]
				

    def create_block_volumes(self, filesystem_type):
	'''
	Create a block volume for to test with blob size files. 
	'''
        #0
	volume_name = "block_volume_ts1173" 
		    
        test_passed = False
        r = None
        port = config.FDS_REST_PORT
        try:

            #Get the user token
            userToken = str(lib.get_user_token("admin", "admin",
                                                 self.om_ip_address, port, 0, 1))
            self.log.info("userToken = %s", userToken)

            #Setup for the request
            url = "http://" + self.om_ip_address + ":" + `port` + "/api/config/volumes"
            self.log.info("url = %s", url)
            header = {'FDS-Auth' : userToken}
            self.log.info("header = %s", header)

            #prep data
            data = {"sla":0,"limit":0,"priority":10,"snapshotPolicies":[],
                    "timelinePolicies":[{"retention":604800,
                    "recurrenceRule":{"FREQ":"DAILY"}},{"retention":1209600,
                    "recurrenceRule":{"FREQ":"WEEKLY"}},{"retention":5184000,
                    "recurrenceRule":{"FREQ":"MONTHLY"}},{"retention":31622400,
                    "recurrenceRule":{"FREQ":"YEARLY"}}],"commit_log_retention":86400,
                    "data_connector":{"type":"BLOCK","api":"Basic, Cinder",
                    "options":{"max_size":"10","unit":["GB","TB","PB"]},
                    "attributes":{"size":10,"unit":"GB"}},"name":volume_name}

            json_data = json.dumps(data)

            #create volume
            r = requests.post(url, data=json_data, headers=header)
            if r is None:
                raise ValueError, "r is None"
                test_passed = False

            else:

                self.log.info("request = %s", r.request)
                self.log.info("response = %s", r.json())
                self.log.info("status = %s", r.status_code)

                #Check return code
                self.assertTrue(200 == r.status_code)

		#create mount point
	        cmds = (
			'./nbdadm.py detach /dev/nbd15',
			'mkdir /fdsmount',
			'./nbdadm.py attach %s %s' %(self.om_ip_address, volume_name),
			'%s /dev/nbd15' %filesystem_type,
			'mount /dev/nbd15 /fdsmount'

			)

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
				'rm -f *.img',
				'./nbdadm.py detach %s' %(volume_name)
			)

		for cmd in cmds:
			self.log.info("Executing %s" %cmd)
			(stdin, stdout, stderr) = self.local_ssh.client.exec_command(cmd)
			self.log.info(stdout.readlines())
				

		#compare hashes for small blob of files moved to /fdsmount with local file hashes
		#If hashes don't match for even a single file, test fails
		for k, v in self.hash_table.iteritems():
		    self.log.info("Comparing hashes for file %s" % k)
		    if self.hash_table[k]  != self.hash_table_fdsmount[k]:
			self.log.warning("fdsmount: %s != local: %s" % (self.hash_table[k], self.hash_table_fdsmount[k]))
			self.test_passed = False
			break

		test_passed = True
		

        except Exception, e:
            self.log.exception(e)

        finally:
	    self.local_ssh.client.close()
            super(self.__class__, self).reportTestCaseResult(test_passed)


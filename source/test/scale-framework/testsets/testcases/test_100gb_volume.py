#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
# author: Philippe Ribeiro
# email: philippe@formationds.com
# file: test_100gb_volume.py

import utils
import config
import unittest
import requests
import json
import sys

import ssh
import os
from boto.s3.key import Key
import testsets.testcase as testcase


class Test100GBVolume(testcase.FDSTestCase):
    '''
    Create one block volume with 100GB capacity, and populate first with 100GB;
    After that, add more 2GB of data, to ensure correctness.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(Test100GBVolume, self).__init__(parameters=parameters,
                                               config_file=config_file,
                                               om_ip_address=om_ip_address)
    
    def runTest(self):
        self.upload_s3_volume()
    
    def upload_s3_volume(self):
        utils.create_dir(config.TEST_DIR)
        # utils.create_dir(config.DOWNLOAD_DIR)
        s3conn = utils.create_s3_connection(self.om_ip_address, self.om_ip_address)
        bucket_name = "volume_100gb"
        bucket = s3conn.conn.create_bucket(bucket_name)
        if bucket == None:
            raise Exception("Invalid bucket.")
            # We won't be waiting for it to complete, short circuit it.
            self.test_passed = False
            self.reportTestCaseResult(self.test_passed)
        self.log.info("Volume %s created..." % bucket.name)
        # Store all the files created to the bucket
        self.store_file_to_volume(bucket)
        # self.log.info("Trying to Download the blobs now.")
        # self.download_files(bucket)
        # self.check_files_hash()
        # Delete the bucket
        # self.destroy_volume(bucket, s3conn)
        # remove the existing file
        utils.remove_dir(config.DOWNLOAD_DIR)
        
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
        sample = "test_sample_200M"
        path = os.path.join(config.TEST_DIR, sample)
        self.log.info("Uploading: %s" % path)
        if os.path.exists(path):
            k.key = sample
            k.set_contents_from_filename(path,
                                         cb=utils.percent_cb,
                                         num_cb=10)
            self.log.info("Uploaded file %s to bucket %s" % 
                         (sample, bucket.name))
                
    def upload_block_volume(self):
        test_passed = False
        r = None
        port = config.FDS_REST_PORT
        try:

            #Get the user token
            userToken = str(utils.get_user_token("admin", "admin",
                                                 self.om_ip_address, port, 0, 1))
            self.log.info("userToken = %s", userToken)

            #Setup for the request
            url = "http://" + self.om_ip_address + ":" + `port` + "/api/config/volumes"
            self.log.info("url = %s", url)
            header = {'FDS-Auth' : userToken}
            self.log.info("header = %s", header)

            #Get the s3 connection from the testcase parameters
            if 's3' not in self.parameters:
                raise ValueError, "S3 connection not present in the parameters"
                test_passed = False
    
            #Get number of volumes currently?
            volume_name = "test-block-100GB"
            #prep data
            data = {"sla":0,"limit":0,"priority":10,"snapshotPolicies":[],
                    "timelinePolicies":[{"retention":604800,
                    "recurrenceRule":{"FREQ":"DAILY"}},{"retention":1209600,
                    "recurrenceRule":{"FREQ":"WEEKLY"}},{"retention":5184000,
                    "recurrenceRule":{"FREQ":"MONTHLY"}},{"retention":31622400,
                    "recurrenceRule":{"FREQ":"YEARLY"}}],"commit_log_retention":86400,
                    "data_connector":{"type":"BLOCK","api":"Basic, Cinder",
                    "options":{"max_size":"100","unit":["GB","TB","PB"]},
                    "attributes":{"size":100,"unit":"GB"}},"name":volume_name}

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
                
                local_ssh = ssh.SSHConn(config.NDBADM_CLIENT, config.SSH_USER,
                                        config.SSH_PASSWORD)
                
                cmds = (
                    'mkdir /fdsmount',
                    './nbdadm.py attach %s %s' % (self.om_ip_address, volume_name),
                    'mkfs.ext4 /dev/nbd15',
                    'mount /dev/nbd15  /fdsmount',
                    'fallocate -l 10G sample_file',
                    'mv sample_file /fdsmount',
                    'umount /fdsmount',
                    'rm -rf /fdsmount',
                    './nbdadm.py detach %s' % (volume_name),
                )
                
                for cmd in cmds:
                    self.log.info("Executing %s" % cmd)
                    (stdin, stdout, stderr) = local_ssh.client.exec_command(cmd)
                    self.log.info(stdout.readlines())
    
                local_ssh.client.close()
                test_passed = True

        except Exception, e:
            self.log.exception(e)
        finally:
            super(self.__class__, self).reportTestCaseResult(test_passed)
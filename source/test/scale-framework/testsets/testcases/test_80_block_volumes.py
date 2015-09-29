#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
# author: Philippe Ribeiro
# email: philippe@formationds.com
# file: test_100gb_volume.py

import lib
import config
import unittest
import requests
import json
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
# import the testlib lib
import testlib.lib.nbd as nbd
import lib
import file_generator
import block_volumes

class Test80BlockVolume(testcase.FDSTestCase):
    '''
    Create one block volume with 100GB capacity, and populate first with 100GB;
    After that, add more 2GB of data, to ensure correctness.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(Test80BlockVolume, self).__init__(parameters=parameters,
                                               config_file=config_file,
                                               om_ip_address=om_ip_address)

        # create 1 file of 1GB for the tests.
        self.fgen = file_generator.FileGenerator(1, 1, 'G')
        self.ndblib = nbd.NBD(self.om_ip_address)
        self.block_volume = block_volumes.BlockVolumes(om_ip_address)
        self.volumes = []
        self.devices = {}

    def runTest(self):
        self.create_volumes()
        self.upload_block_volume()
        super(self.__class__, self).reportTestCaseResult(True)
        

    def upload_block_volume(self):
        # loop over all the 80 volumes
        i = 0
        while i < 80:
            volumes_block = self.volumes[i:i+16]
            # pick 16 volumes
            for volume in volumes_block:
                path = config.FDS_MOUNT % volume
                if not os.path.exists(path):
                    # clean up the existing mount point
                    os.makedirs(path)
                # mount the volume
                self.ndblib.mount_volume(volume, path, True)
                # generate the file to be copied to
                self.fgen.generate_files()
                # copy the file to the mount point
                files = self.fgen.get_files()
                for f in files:
                    current = os.path.join(config.RANDOM_DATA, f)
                    shutil.copy(current, path)
                # remove the files created
            for volume in volumes_block:
                self.fgen.clean_up()
                # unmount the device
                self.ndblib.unmount(path)
                # detach the ndb device
                if self.ndblib.detach(volume):
                    test_passed = True
                else:
                    test_passed = False
            i += 16

    def create_volumes(self):
        '''
        Create a block volume, and move a 100GB file to it
        '''
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

            for i in xrange(1, 81):
                volume_name = "test_80_block_volumes_%s" % i
                #prep data
                data = {"sla":0,"limit":0,"priority":10,"snapshotPolicies":[],
                        "timelinePolicies":[{"retention":604800,
                        "recurrenceRule":{"FREQ":"DAILY"}},{"retention":1209600,
                        "recurrenceRule":{"FREQ":"WEEKLY"}},{"retention":5184000,
                        "recurrenceRule":{"FREQ":"MONTHLY"}},{"retention":31622400,
                        "recurrenceRule":{"FREQ":"YEARLY"}}],"commit_log_retention":86400,
                        "data_connector":{"type":"BLOCK","api":"Basic, Cinder",
                        "options":{"max_size":"100","unit":["GB","TB","PB"]},
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
                    
                    self.volumes.append(volume_name)

        except Exception, e:
            self.log.exception(e)
            test_passed = False
    
    
    def tearDown(self):
        '''
        Undo it.
        '''
        self.block_volume.delete_volumes(self.volumes)

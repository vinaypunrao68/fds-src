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
import time
import shutil

import ssh
import os
import utils

nbd_path = os.path.abspath(os.path.join('..', ''))
sys.path.append(nbd_path)

from boto.s3.key import Key

import testsets.testcase as testcase
# import the testlib utils
import testlib.utils.nbd as nbd
import utils
import file_generator
import block_volumes

class TestBareAMCrash(testcase.FDSTestCase):
    '''
    Create one block volume with 100GB capacity, and populate first with 100GB;
    After that, add more 2GB of data, to ensure correctness.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestBareAMCrash, self).__init__(parameters=parameters,
                                               config_file=config_file,
                                               om_ip_address=om_ip_address)

        # create 1 file of 1GB for the tests.
        self.fgen = file_generator.FileGenerator(1, 1, 'G')
        self.ndblib = nbd.NBD(self.om_ip_address)
        self.block_volume = block_volumes.BlockVolumes(om_ip_address)
        self.volumes = []
        self.devices = {}

    def runTest(self):
        self.volumes = self.block_volume.create_volumes(2, "test_bare_am", 10,
                                                        "GB")
        test_passed = False
        table = {}
        for volume in self.volumes:
            path = config.FDS_MOUNT % volume
            table[volume] = path
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
        # remove the files which were created
        self.fgen.clean_up()
        
        for volume, path in table.iteritems():
            # unmount the device
            self.ndblib.unmount(path)
            # detach the ndb device
            if self.ndblib.detach(volume):
                test_passed = True
            else:
                test_passed = False
                self.reportTestCaseResult(test_passed)
        self.reportTestCaseResult(test_passed)
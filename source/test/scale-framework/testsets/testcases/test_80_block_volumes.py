#!/usr/bin/python
#
# Copyright: 2015 by Formation Data Systems, Inc.
# author: Philippe Ribeiro
# email: philippe@formationds.com
# file: test_80_block_volumes.py

import utils
import config
import unittest
import requests
import json
import sys
import time

import ssh
import os
import utils

from boto.s3.key import Key

import testsets.testcase as testcase
import utils
import file_generator
import block_volumes

class Test80BlockVolumes(testcase.FDSTestCase):
    '''
    In this test we create 80 block volumes. Since there are 16 ndb devices,
    we rotate those volumes by first attaching 16 volumes to 16 ndb devices,
    copying a 10MB sample file, checking for data consistency, detaching the
    ndb device from the volume, and moving to the next set of 16 devices.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(Test80BlockVolumes, self).__init__(parameters=parameters,
                                                 config_file=config_file,
                                                 om_ip_address=om_ip_address)
        # create a one 10M file for sampling
        self.fgen = file_generator.FileGenerator(10, 1, 'M')
        self.bv = block_volumes.BlockVolumes(self.om_ip_address)
        self.volumes = self.bv.create_volumes(80, "ndb_device", 100, "GB")

    def runTest(self):
        for volume in self.volumes:
            self.log.info(volume)
        
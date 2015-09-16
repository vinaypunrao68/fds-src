#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
# author: Philippe Ribeiro
# email: philippe@formationds.com
# file: test_multiple_block_mount_points.py

import lib
import config
import unittest
import random
import requests
import json
import sys

import ssh
import testsets.testcase as testcase


class TestMultipleBlockMountPoints(testcase.FDSTestCase):
    '''
    Test the creation of the maximum number of volumes via concurrency,
    in order to test show well the system supports multithreading.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestMultipleBlockMountPoints, self).__init__(parameters=parameters,
                                                    config_file=config_file,
                                                    om_ip_address=om_ip_address)
        
    def runTest(self):
        
        # create 10 mount points
        sample_block_volumes = random.sample(range(1, config.MAX_NUM_VOLUMES), 10)
        local_ssh = ssh.SSHConn(config.NDBADM_CLIENT, config.SSH_USER,
                                config.SSH_PASSWORD)
        test_passed = False
        try:
            for i in sample_block_volumes:
                volume_name = "test-block-%s" % i
                self.log.info("Mounting block volume: %s" % volume_name)
                cmds = (
                    #'umount /fdsmount',
                    'rm -rf /fdsmount*',
                    'rm -rf sample_file',
                    'mkdir /fdsmount_%s' % i,
                    #'./nbdadm.py detach %s' % (volume_name),
                    './nbdadm.py attach %s %s' % (self.om_ip_address, volume_name),
                    'mkfs.ext4 /dev/nbd%s' % i,
                    'mount /dev/nbd15  /fdsmount_%s' % i,
                    'fallocate -l 20M sample_file',
                    'mv sample_file /fdsmount_%s' % i,
                )
                
                for cmd in cmds:
                    (stdin, stdout, stderr) = local_ssh.client.exec_command(cmd)
                    self.log.info(stdout.readlines())
                test_passed = True
        except Exception, e:
            self.log.exception(e)
            test_passed = False
        finally:
            super(self.__class__, self).reportTestCaseResult(test_passed)
            local_ssh.client.close()
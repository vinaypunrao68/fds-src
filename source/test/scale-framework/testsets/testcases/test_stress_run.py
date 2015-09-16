#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.

import ConfigParser
import os
import shutil
import random
import lib
import config
import unittest
import json
import testsets.testcase as testcase
import block_volumes
import s3_volumes
import logging

from datetime import datetime
from datetime import timedelta
from threading import Thread
from time import sleep

class TestStressRun(testcase.FDSTestCase):

    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)

    '''
    Setup volumes for the stress test
    '''

    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestStressRun, self).__init__(parameters=parameters,
                config_file=config_file, om_ip_address=om_ip_address)

        self.s3_vol_count = 500
        self.block_vol_count = 500
        self.should_be_running = True
        self.endtime = datetime.now() + timedelta(hours=6)
        if(config_file != None):
            self.log.info("config_file = %s", config_file)
            config = ConfigParser.ConfigParser()
            config.readfp(open(config_file))
            # TODO: Only get this if these values are set
            self.s3_vol_count = config.getint("stress", "s3_vols")
            self.block_vol_count = config.getint("stress", "block_vols")


    '''
    Do it.
    '''
    def runTest(self):
        test_passed = False
        s3_vols = s3_volumes.S3Volumes(self.om_ip_address)
        block_vols = block_volumes.BlockVolumes(self.om_ip_address)

        try:
            #Run Stress

            #S3 in a thread
            s3_thread = Thread(target = self.threaded_s3_operations, args=())
            s3_thread.start()

            #Block in a thread
            block_thread = Thread(target = self.threaded_block_operations, args=())
            block_thread.start()

            #wait for hours
            while(self.still_running()):
                #TODO: check threads for errors?
                sleep(30)

            #tell threads to finish
            self.should_be_running = False

            #wait for threads to finish
            s3_thread.join()
            block_thread.join()

            #report stats? How? To a file? Xml? through xunit?

            #Yay?
            test_passed = True


        except Exception, e:
            self.log.exception(e)
            test_passed = False
        finally:
            super(self.__class__, self).reportTestCaseResult(test_passed)

    '''
    Undo it.
    '''
#    def tearDown(self):

    def still_running(self):
        running = False
        if(datetime.now() < self.endtime):
            running = True
        return running

    def threaded_s3_operations(self):
        s3_vols = s3_volumes.S3Volumes(self.om_ip_address)
        #shutil.rmtree("stress_test_write")
        #shutil.rmtree("stress_test_read")
        os.makedirs("stress_test_write")
        os.makedirs("stress_test_read")
        volumes = []
        for i in xrange(0, self.s3_vol_count):
            name = "stress_test_s3_" + `i`
            try:
                file = open("stress_test_write/"+name,'w')
                file.write(name + "\n")
                file.close()
                volumes.append(name)
            except:
                print('failed to create file')

        count = 0
        write_count = 0
        write_failed = 0
        while(self.should_be_running):
            for volume_name in volumes:
                file_name = "stress_test_write/"+volume_name
                file = open(file_name, 'a')
                file.write('\n' + str(count))
                file.close()
                try:
                    write_count = write_count + 1
                    volume = s3_vols.get_bucket(volume_name)
                    s3_vols.store_file_to_volume(volume, file_name, volume.name + "_key")
                except:
                    write_failed = write_failed + 1
                if not self.should_be_running:
                    break
            count = count + 1
            print("loop count: {}".format(count))
            print("writes success rate: {}%".format(
                (write_count - write_failed) / write_count * 100))
            #continue
        shutil.rmtree("stress_test_write")
        shutil.rmtree("stress_test_read")

    def threaded_block_operations(self):
        while(self.should_be_running):
            #print(".")
            sleep(2)
            #continue

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()

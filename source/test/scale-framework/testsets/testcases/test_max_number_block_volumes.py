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

import fds_engine
import ssh
import testsets.testcase as testcase
import block_volumes

class TestMaxNumberBlockVolumes(testcase.FDSTestCase):
    '''
    Test the creation of the maximum number of volumes via concurrency,
    in order to test show well the system supports multithreading.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestMaxNumberBlockVolumes, self).__init__(parameters=parameters,
                                                    config_file=config_file,
                                                    om_ip_address=om_ip_address)
        self.engine = fds_engine.FdsEngine(parameters['inventory_file'])
        self.volumes = []
        self.block_volume = block_volumes.BlockVolumes(om_ip_address)
        
    def runTest(self):
        '''
        Use the lib do_work multithreaded utility
        '''
        # make sure cluster is cleaned and restart before
        self.engine.stop_all_nodes_processes()
        self.engine.start_all_nodes_processes()
        mylist = list(xrange(config.MAX_NUM_VOLUMES))
        lib.do_work(self.create_volumes, mylist)
        lib.do_work(self.block_volume.delete_block_volume, self.volumes)
    
    def create_volumes(self, index):
        '''
        Responsible to create a block volume, via the API call, and name it
        based on the index given.
        
        Attributes:
        -----------
        index: integer
            the index of the volume being created
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

            #Get the s3 connection from the testcase parameters
            if 's3' not in self.parameters:
                raise ValueError, "S3 connection not present in the parameters"
                test_passed = False
                        
            volume_name = "test-block-%s" % index
            # Create a 1GB block volume
            data = {"sla":0,"limit":0,"priority":10,"snapshotPolicies":[],
                    "timelinePolicies":[{"retention":604800,
                    "recurrenceRule":{"FREQ":"DAILY"}},{"retention":1209600,
                    "recurrenceRule":{"FREQ":"WEEKLY"}},{"retention":5184000,
                    "recurrenceRule":{"FREQ":"MONTHLY"}},{"retention":31622400,
                    "recurrenceRule":{"FREQ":"YEARLY"}}],"commit_log_retention":86400,
                    "data_connector":{"type":"BLOCK","api":"Basic, Cinder",
                    "options":{"max_size":"100","unit":["GB","TB","PB"]},
                    "attributes":{"size":1,"unit":"GB"}},"name":volume_name}
            self.volumes.append(volume_name)

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
            test_passed = True


        except Exception, e:
            self.log.exception(e)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

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

import testsets.testcase as testcase


class Test100GBVolume(testcase.FDSTestCase):
    
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(Test100GBVolume, self).__init__(parameters=parameters,
                                               config_file=config_file,
                                               om_ip_address=om_ip_address)
        
    def runTest(self):
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
            conn = self.parameters['s3'].get_s3_connection()

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
    
                    #Write to the volume
    
                    #Read from volume
    
                #Get Volumes
                #r = requests.get(url, headers=header)
                #self.log.info("response = %s", r.json())
                #self.log.info("Status = %s", r.status_code)
                #Yay?
                test_passed = True


        except Exception, e:
            self.log.exception(e)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)
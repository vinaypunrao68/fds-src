#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.

import s3
import utils
import config
import unittest
import requests
import json
import testsets.testcase as testcase

from boto.s3.key import Key
from boto.s3.bucket import Bucket

class TestCreateThreehundredObjectVolumes(testcase.FDSTestCase):

    '''
    Test that 300 object volumes can successfully be created via the REST api
    '''

    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestCreateThreehundredObjectVolumes, self).__init__(parameters=parameters,
                config_file=config_file, om_ip_address=om_ip_address)

    '''
    Do it.
    '''
    def runTest(self):
        test_passed = False
        r = None
        port = config.FDS_REST_PORT

        try:

            #Get the user token
            userToken = str(utils.get_user_token("admin", "admin", self.om_ip_address, port, 0, 1))
            self.log.info("userToken = %s", userToken)

            #Setup for the request
            url = "http://" + self.om_ip_address + ":" + `port` + "/api/config/volumes"
            self.log.info("url = %s", url)
            header = {'FDS-Auth' : userToken}
            self.log.info("header = %s", header)

            #Get the s3 connection from the testcase parameters
            conn = self.parameters['s3'].get_s3_connection()

            #Get number of volumes currently?

            for i in range(0, 300):

                #bucket name
                bucket_name = "test-" + str(i).zfill(3) + "_300"

                #prep data
                data = {"sla":0,"limit":0,"priority":10,"snapshotPolicies":[],
                        "timelinePolicies":[{"retention":604800,"recurrenceRule":{"FREQ":"DAILY"}},
                        {"retention":1209600,"recurrenceRule":{"FREQ":"WEEKLY"}},{"retention":5184000,
                        "recurrenceRule":{"FREQ":"MONTHLY"}},{"retention":31622400,
                        "recurrenceRule":{"FREQ":"YEARLY"}}],"commit_log_retention":86400,
                        "data_connector":{"type":"OBJECT","api":"S3, Swift"},"name":bucket_name,
                        "mediaPolicy":"HDD_ONLY"}

                json_data = json.dumps(data)

                #create volume
                r = requests.post(url, data=json_data, headers=header)
                self.log.info("request = %s", r.request)
                self.log.info("response = %s", r.json())
                self.log.info("status = %s", r.status_code)

                #Check return code
                self.assertTrue(200 == r.status_code)

                #Get the bucket via boto
                #bucket = conn.lookup(bucket_name)
                #self.assertTrue(bucket != None)
                #self.log.info("bucket = %s", bucket)

                #Write to the volume
                #k = Key(bucket)
                #self.log.info("key = %s", k)
                #k.key = "test"
                #in_string = "This is a test."
                #k.set_contents_from_string = in_string

                #Read from volume
                #self.log.info("keys = %s", bucket.get_all_keys())
                #k = bucket.get_key("test")
                #out_string = k.get_contents_as_string()
                #self.assertTrue(in_string == out_string)

            #Get Volumes
            r = requests.get(url, headers=header)
            self.log.info("response = %s", r.json())
            self.log.info("Status = %s", r.status_code)

            #Yay?
            test_passed = True


        except:
            self.log.info("response = %s", r.json())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

    '''
    Undo it.
    '''
#    def tearDown(self):

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()

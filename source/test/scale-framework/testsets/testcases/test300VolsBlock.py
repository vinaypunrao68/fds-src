#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.

import utils
import config
import unittest
import requests
import json
import testsets.testcase as testcase
import block_volumes

class TestCreateThreehundredBlockVolumes(testcase.FDSTestCase):

    '''
    Test that 300 object volumes can successfully be created via the REST api
    '''

    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestCreateThreehundredBlockVolumes, self).__init__(parameters=parameters,
                config_file=config_file, om_ip_address=om_ip_address)
        
        self.volumes = []
        self.block_volume = block_volumes.BlockVolumes(om_ip_address)

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

            for i in range(0, 100):

                #volume name
                volume_name = "test-block-" + str(i).zfill(3) + "_300"
                self.volumes.append(volume_name)
                #prep data
                data = {"sla":0,"limit":0,"priority":10,"snapshotPolicies":[],
                        "timelinePolicies":[{"retention":604800,
                        "recurrenceRule":{"FREQ":"DAILY"}},{"retention":1209600,
                        "recurrenceRule":{"FREQ":"WEEKLY"}},{"retention":5184000,
                        "recurrenceRule":{"FREQ":"MONTHLY"}},{"retention":31622400,
                        "recurrenceRule":{"FREQ":"YEARLY"}}],"commit_log_retention":86400,
                        "data_connector":{"type":"BLOCK","api":"Basic, Cinder",
                        "options":{"max_size":"100","unit":["GB","TB","PB"]},
                        "attributes":{"size":1,"unit":"GB"}},"name":volume_name}

                json_data = json.dumps(data)

                #create volume
                r = requests.post(url, data=json_data, headers=header)
                if r is None:
                    raise ValueError, "r is None"
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
        finally:
            super(self.__class__, self).reportTestCaseResult(test_passed)
            
    def tearDown(self):
        '''
        Undo it.
        '''
        self.block_volume.delete_volumes(self.volumes)

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()

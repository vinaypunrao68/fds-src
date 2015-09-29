import os
import sys
import unittest

from boto.s3.key import Key

import config
import config_parser
import users
import lib
import samples
import s3
import testsets.testcase as testcase
import block_volumes

class TestMixedBlocks(testcase.FDSTestCase):
    
    '''
    Test the scenario where half of the volumes are of the type S3 buckets,
    and half of the type block volumes. Upload 10MB of data to each and
    check for data consistency.
    '''
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestMixedBlocks, self).__init__(parameters=parameters,
                                              config_file=config_file,
                                              om_ip_address=om_ip_address)
        
        self.ip_addresses = config_parser.get_ips_from_inventory(
                                                            self.inventory_file)
        self.buckets = []
        self.s3_connections = []
        self.blocks = []
    
    @unittest.expectedFailure
    def runTest(self):
        '''
        Run the test
        '''
        # establish a S3 connection
        s3conn = self.connect_s3()
        # create 500 s3 volumes
        self.create_s3_volumes(s3conn)
        # delete the existing S3 volumes.
        self.delete_volumes(s3conn)
        self.create_block_volumes()
        self.delete_block_volumes()


    def delete_block_volumes(self):
        for block in self.blocks:
            block_volumes.delete_block_volume(block)

    def connect_s3(self):
        '''
        establish a S3 connection
        '''
        s3conn = s3.S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            None,
            self.om_ip_address,
            config.FDS_S3_PORT,
            self.om_ip_address,
        )
        s3conn.s3_connect()
        return s3conn

    def create_s3_volumes(self, s3conn):
        '''
        This particular method is responsible for creating 500 S3 buckets.
        They will represet the S3 part of the test.
        
        Arguments:
        ----------
        s3conn : the s3 connection instance
        '''
        bucket_name = "s3_volume_%s"
        for i in xrange(0, 500):
            bucket = s3conn.conn.create_bucket(bucket_name % i)
            if bucket == None:
                raise Exception("Invalid bucket.")
                # We won't be waiting for it to complete, short circuit it.
                self.test_passed = False
                self.reportTestCaseResult(self.test_passed)
            
            self.log.info("Volume %s created..." % bucket.name)
            self.buckets.append(bucket)
            
            # After creating volume and uploading the sample files to it
            # ensure data consistency by hashing (MD5) the file and comparing
            # with the one stored in S3
        self.test_passed = True
    
    def delete_volumes(self, s3conn):
        '''
        After test executes, remove existing buckets.
        
        Attributes:
        -----------
        s3conn : S3Conn
            the connection object to the FDS instance via S3
            
        '''
        for bucket in self.buckets:
            if bucket:
                for key in bucket.list():
                    bucket.delete_key(key)
                self.log.info("Deleting bucket: %s", bucket.name)
                s3conn.conn.delete_bucket(bucket.name)
        self.buckets = []
    
    def create_block_volumes(self):
        '''
        Responsible for creating 500 block volumes, using the REST API.
        Each block volume will be populated with 10MB of data, and this test
        represent the Block volume half of the test.
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
    
            #Get number of volumes currently?
            volume_name = "block_volume_%s"
            for i in xrange(0, 500):
                #prep data
                name = volume_name % i
                data = {"sla":0,"limit":0,"priority":10,"snapshotPolicies":[],
                        "timelinePolicies":[{"retention":604800,
                        "recurrenceRule":{"FREQ":"DAILY"}},{"retention":1209600,
                        "recurrenceRule":{"FREQ":"WEEKLY"}},{"retention":5184000,
                        "recurrenceRule":{"FREQ":"MONTHLY"}},{"retention":31622400,
                        "recurrenceRule":{"FREQ":"YEARLY"}}],"commit_log_retention":86400,
                        "data_connector":{"type":"BLOCK","api":"Basic, Cinder",
                        "options":{"max_size":"100","unit":["GB","TB","PB"]},
                        "attributes":{"size":100,"unit":"GB"}},"name": name}
                self.blocks.append(name) 
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
        finally:
            super(self.__class__, self).reportTestCaseResult(test_passed)

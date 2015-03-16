import os
import sys

from boto.s3.key import Key

import config
import config_parser
import users
import utils
import samples
import s3
import testsets.testcase as testcase

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
        self.hash_table = {}
        self.sample_files = []
        self.s3_connections = []
    
        utils.create_dir(config.TEST_DIR)
        utils.create_dir(config.DOWNLOAD_DIR)
            
        # for this test, we will create 5 sample files, 2MB each
        for current in samples.sample_mb_files[:3]:
            path = os.path.join(config.TEST_DIR, current)
            if os.path.exists(path):
                self.sample_files.append(current)
                encode = utils.hash_file_content(path)
                self.hash_table[current] = encode
        self.log.info("hash table: %s" % self.hash_table)
        
    def runTest(self):
        # establish a S3 connection
        s3conn = self.connect_s3()
        # create 500 s3 volumes
        self.create_s3_volumes(s3conn)
        # delete the existing S3 volumes.
        self.delete_volumes(s3conn)

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
            self.store_file_to_volume(bucket)
            
            # After creating volume and uploading the sample files to it
            # ensure data consistency by hashing (MD5) the file and comparing
            # with the one stored in S3
            self.download_files(bucket)
            
            for k, v in self.hash_table.iteritems():
                # hash the current file, and compare with the key
                self.log.info("Hashing for file %s" % k)
                path = os.path.join(config.DOWNLOAD_DIR, k)
                hashcode = utils.hash_file_content(path)
                if v != hashcode:
                    self.log.warning("%s != %s" % (v, hashcode))
                    self.test_passed = False
                    self.reportTestCaseResult(self.test_passed)
            self.test_passed = True
    
    def download_files(self, bucket):
        utils.create_dir(config.DOWNLOAD_DIR)
        bucket_list = bucket.list()
        for l in bucket_list:
            key_string = str(l.key)
            path = os.path.join(config.DOWNLOAD_DIR, key_string)
            # check if file exists locally, if not: download it
            if not os.path.exists(path):
                self.log.info("Downloading %s" % path)
                l.get_contents_to_filename(path)
                
    def store_file_to_volume(self, bucket):
        '''
        Given the list of files to be uploaded, presented in sample_files list,
        upload them to the corresponding volume
        
        Attributes:
        -----------
        bucket : bucket
            the S3 bucket (volume) where the data files will to uploaded to.
        '''
        # add the data files to the bucket.
        k = Key(bucket)
        for sample in self.sample_files:
            path = os.path.join(config.TEST_DIR, sample)
            if os.path.exists(path):
                k.key = sample
                k.set_contents_from_filename(path,
                                             cb=utils.percent_cb,
                                             num_cb=10)
                self.log.info("Uploaded file %s to bucket %s" % 
                             (sample, bucket.name))
                
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
        finally:
            super(self.__class__, self).reportTestCaseResult(test_passed)
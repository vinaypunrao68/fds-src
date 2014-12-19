import unittest
import s3

import config

def TestS3Conn(unittest):
    
    def setUp(self):
        self.s3conn = s3.S3Connection(
                config.FDS_DEFAULT_KEY_ID,
                config.FDS_DEFAULT_SECRET_ACCESS_KEY,
                config.FDS_DEFAULT_HOST,
                config.FDS_AUTH_DEFAULT_PORT,       
              )
import boto

from filechunkio import FileChunkIO
from boto.s3 import connection
from boto.s3.key import Key

import testsets.testcase as testcase

# Class to contain S3 objects used by these test cases.
class S3(object):
    def __init__(self, conn):
        self.conn = conn
        self.bucket1 = None
        self.keys = []  # As we add keys to the bucket, add them here as well.
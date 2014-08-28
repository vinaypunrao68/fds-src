#!/usr/bin/python
import os
import sys
import math
import argparse
import random
import boto
import unittest
import time
from boto.s3.connection import OrdinaryCallingFormat
from filechunkio import FileChunkIO

DEBUG = False
HOST = 'localhost'
PORT = 8000
BUCKET_NAME = 'bucket'
DATA_FILE = '/mup_rand'

random.seed(time.time())

def log(text, output=sys.stdout):
    if DEBUG:
        print >> output, text

class TestMultiUpload(unittest.TestCase):
    '''
     Tests multiupload functionality on FDS
     Assumes that FDS is already running locally
    '''
    def setUp(self):
        # Connect to FDS S3 interface
        self.c = boto.connect_s3(
                aws_access_key_id='blablabla',
                aws_secret_access_key='kekekekek',
                host=HOST,
                port=PORT,
                is_secure=False,
                calling_format=boto.s3.connection.OrdinaryCallingFormat())
        log('Connected!')

        # Create bucket
        #TODO: Randomized bucket names are a temporary workaround for dodgy bucket delete on the server side.
        self._bucket_name = BUCKET_NAME + str(random.randint(0, 100000))
        self.b = self.c.create_bucket(self._bucket_name)
        log('Bucket <%s> created!' % self._bucket_name)

        # Get file info
        self.source_path = DATA_FILE
        self.source_size = os.stat(self.source_path).st_size
        log('source size: ' + str(self.source_size))

    def test_multi_upload_to_completion(self):
        # Create multipart upload request
        _key_name = os.path.basename(self.source_path)
        mp = self.b.initiate_multipart_upload(_key_name)
        log('MPU initiated!')

        # Use a chunk size of 50 MB
        chunk_size = 52428800
        chunk_count = int(math.ceil(self.source_size / chunk_size))

        # Send the file parts, using FileChunkIO to create a file-like object
        # that points to a certain byte range within the original file. We
        # set bytes to never exceed the original file size.
        for i in range(chunk_count + 1):
            offset = chunk_size * i
            bytes = min(chunk_size, self.source_size - offset)
            with FileChunkIO(self.source_path, 'r', offset=offset, bytes=bytes) as fp:
                mp.upload_part_from_file(fp, part_num=i + 1)
        log('MPU Pieces Uploaded!')

        # Finish the upload
        mp.complete_upload()
        log('MPU Complete!')

    def test_multi_upload_abort(self):
        # Create multipart upload request
        _key_name = os.path.basename(self.source_path)
        mp = self.b.initiate_multipart_upload(_key_name)
        log('MPU initiated!')

        # Use a chunk size of 50 MB
        chunk_size = 52428800
        chunk_count = int(math.ceil(self.source_size / chunk_size))

        # Send the file parts, using FileChunkIO to create a file-like object
        # that points to a certain byte range within the original file. We
        # set bytes to never exceed the original file size.
        # For this particular test case we will stop halfway
        for i in range((chunk_count / 2) + 1):
            offset = chunk_size * i
            bytes = min(chunk_size, self.source_size - offset)
            with FileChunkIO(self.source_path, 'r', offset=offset, bytes=bytes) as fp:
                mp.upload_part_from_file(fp, part_num=i + 1)

        #TODO: Waiting until bug is fixed upstream, temporary workaround
        try:
            self.b.cancel_multipart_upload(_key_name, mp.id)
        except Exception as e:
            print e

    def tearDown(self):
        log('Beginning teardown...')
        self.b.delete_keys(self.b.get_all_keys())
        log('Keys successfully deleted')

        #TODO: Waiting until bug is fixed upstream, temporary workaround
        try:
            self.b.delete()
        except Exception as e:
            print e
            log('Bucket successfully deleted!')

        self.c.close()
        log('Teardown complete!')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', action='store_true', help='Enables verbose mode')
    args = parser.parse_args()
    DEBUG = args.v

    suite = unittest.TestLoader().loadTestsFromTestCase(TestMultiUpload)
    unittest.TextTestRunner(verbosity=2).run(suite)

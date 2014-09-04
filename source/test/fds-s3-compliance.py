#!/usr/bin/python
import os
import sys
import math
import argparse
import random
import boto
import unittest
import time
import hashlib
from boto.s3.connection import OrdinaryCallingFormat
from boto.s3.key import Key
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
        global c
        # Create bucket
        #TODO: Randomized bucket names are a temporary workaround for dodgy bucket delete on the server side.
        self._bucket_name = BUCKET_NAME + str(random.randint(0, 100000))
        self.b = c.create_bucket(self._bucket_name)
        log('Bucket <%s> created!' % self._bucket_name)

        # Get file info
        self.source_path = DATA_FILE
        self.source_size = os.stat(self.source_path).st_size
        log('source size: ' + str(self.source_size))

    def test_list_all_buckets(self):
        global c
        log('Retrieving list of all buckets')
        rs = c.get_all_buckets()
        assert(len(rs) > 0)
        for b in rs:
            log(b.name)

    def test_put_copy(self):
        global c
        _aux_bucket_name = BUCKET_NAME + str(random.randint(0, 100000));
        log('Creating auxiliary bucket %s' % _aux_bucket_name)
        _aux_b = c.create_bucket(_aux_bucket_name)
        log('Bucket %s created!' % _aux_bucket_name)
        time.sleep(1)

        # Upload a key
        log('Uploading test key...')
        _key = Key(self.b)
        _key.key = 'putCopyTestObj'
        _key.set_contents_from_filename(DATA_FILE)

        # Copy key
        log('Copying key to aux bucket')
        _aux_b.copy_key('copiedTestObj', self.b.name, _key.key)

        _copied_key = Key(_aux_b)
        _copied_key.key = 'copiedTestObj'
        _copied_key.get_contents_to_filename('/copied_output', 'wb')
        assert(hashlib.sha1(open(DATA_FILE, 'rb')).hexdigest() == hashlib.sha1(open('/copied_output', 'rb')).hexdigest())
        log('Copy successful!')

        # Cleanup
        os.system('rm /copied_output')
        for _k in _aux_b.list():
            _k.delete()
        _aux_b.delete()

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

        # Verify the upload
        k = Key(self.b)
        k.key = _key_name
        k.get_contents_to_filename(DATA_FILE+'_verify')
        diff_code = os.system('diff ' + DATA_FILE + ' ' + DATA_FILE + '_verify')
        if diff_code != 0:
            raise Exception('File corrupted!')

        # Cleanup
        os.system('rm ' + DATA_FILE + '_verify')

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
            log(e)

        try:
            k = Key(self.b)
            k.key = _key_name
            k.get_contents_as_string()
        except: pass
        else:
            raise Exception('Found a key when it should not exist!')

    # def test_multi_upload_list(self):
    #     # Create multipart upload request
    #     _key_name = os.path.basename(self.source_path)
    #     mp = self.b.initiate_multipart_upload(_key_name)
    #     log('MPU initiated!')
    #
    #     # Use a chunk size of 50 MB
    #     chunk_size = 52428800
    #     chunk_count = int(math.ceil(self.source_size / chunk_size))
    #
    #     # Send the file parts, using FileChunkIO to create a file-like object
    #     # that points to a certain byte range within the original file. We
    #     # set bytes to never exceed the original file size.
    #     # For this particular test case we will stop halfway
    #     for i in range((chunk_count / 2) + 1):
    #         offset = chunk_size * i
    #         bytes = min(chunk_size, self.source_size - offset)
    #         with FileChunkIO(self.source_path, 'r', offset=offset, bytes=bytes) as fp:
    #             mp.upload_part_from_file(fp, part_num=i + 1)
    #     log('MPU Pieces Uploaded!')
    #
    #     parts_list = self.b.list_multipart_uploads()
    #     for part in parts_list:
    #         log(str(part))
    #     log('Listing complete!')

    def tearDown(self):
        log('Beginning teardown...')
        for key in self.b.list():
            try:
                key.delete()
            except: pass
        log('Keys successfully deleted')

        #TODO: Waiting until bug is fixed upstream, temporary workaround
        try:
            self.b.delete()
        except Exception as e:
            log(str(e))
            log('Bucket successfully deleted!')

        log('Teardown complete!')

if __name__ == '__main__':
    global c
    # Connect to FDS S3 interface
    c = boto.connect_s3(
        aws_access_key_id='blablabla',
        aws_secret_access_key='kekekekek',
        host=HOST,
        port=PORT,
        is_secure=False,
        calling_format=boto.s3.connection.OrdinaryCallingFormat())
    log('Connected!')

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', action='store_true', help='Enables verbose mode')
    args = parser.parse_args()
    DEBUG = args.v

    suite = unittest.TestLoader().loadTestsFromTestCase(TestMultiUpload)
    unittest.TextTestRunner(verbosity=2).run(suite)

    c.close()
    log('All tests complete!')

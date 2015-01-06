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
from boto.s3.bucket import Bucket
from filechunkio import FileChunkIO

DEBUG = False
HOST = 'localhost'
PORT = 8000
BUCKET_NAME = 'bucket'
DATA_FILE = '/tmp/mup_rand'
SMALL_DATA_FILE = '/tmp/mup_rand_small'

random.seed(time.time())


class TestMultiUpload(testcase.FDSTestCase):
    '''
     Tests multiupload functionality on FDS
     Assumes that FDS is already running locally
    '''
    def __init__(self, parameters=None):
        super(TestMultiUpload, self).__init__(parameters)
        # Create bucket
        #TODO: Randomized bucket names are a temporary workaround for dodgy bucket delete on the server side.
        self._bucket_name = BUCKET_NAME + str(random.randint(0, 100000))
        self.b = c.create_bucket(self._bucket_name)
        time.sleep(2)
        self.self.log.info.info('Bucket <%s> created!' % self._bucket_name)

        # Get file info
        self.source_path = DATA_FILE
        self.source_size = os.stat(self.source_path).st_size
        self.log.info('source size: ' + str(self.source_size))

    def test_list_all_buckets(self):
        global c
        self.log.info('Retrieving list of all buckets')
        rs = c.get_all_buckets()
        assert(len(rs) > 0)
        for b in rs:
            self.log.info(b.name)

    # # Not supported for Goldman release
    # def test_list_bucket_objects_prefix(self):
    #     global c
    #     self.log.info('Uploading small object to %s' % self._bucket_name)
    #     k = Key(self.b)
    #     k.key = 'pref/test_key'
    #     k.set_contents_from_filename(SMALL_DATA_FILE)
    #
    #     self.log.info('Listing keys...')
    #     list = self.b.list(prefix='pref/')
    #     for key in list:
    #         self.log.info(key.key)

    def test_basic_head_object(self):
        global c
        self.log.info('Putting an object')
        _key = Key(self.b)
        _key.key = 'test_key'
        _key.set_contents_from_filename(SMALL_DATA_FILE)

        self.log.info('Checking for object with HEAD')
        _recv = self.b.get_key(_key.key)
        assert(_recv is not None)

    def test_list_keys(self):
        global c
        self.log.info('Putting 4 objects...')

        for i in range(4):
            _k = Key(self.b)
            _k.key = 'test_key' + str(i)
            _k.set_contents_from_filename(SMALL_DATA_FILE)

        # Verify keys
        self.log.info('Verifying keys')
        for i in range(4):
            _k = Key(self.b)
            _k.key = 'test_key' + str(i)
            _k.get_contents_to_filename('/tmp/test_verify' + str(i))
            assert(hashlib.sha1(open('/tmp/test_verify' + str(i), 'rb').read()).hexdigest() == hashlib.sha1(open(SMALL_DATA_FILE).read()).hexdigest())

        # List the keys
        for key in self.b.list():
            self.log.info(str(key))

    def test_put_copy(self):
        global c
        _aux_bucket_name = BUCKET_NAME + str(random.randint(0, 100000))
        self.log.info('Creating auxiliary bucket %s' % _aux_bucket_name)
        _aux_b = c.create_bucket(_aux_bucket_name)
        self.log.info('Bucket %s created!' % _aux_bucket_name)

        # Upload a key
        self.log.info('Uploading test key...')
        _key = Key(self.b)
        _key.key = 'putCopyTestObj'
        _key.set_contents_from_filename(SMALL_DATA_FILE)

        # Copy key
        self.log.info('Copying key to aux bucket')
        _aux_b.copy_key('copiedTestObj', self.b.name, _key.key)

        _copied_key = Key(_aux_b)
        _copied_key.key = 'copiedTestObj'
        _copied_key.get_contents_to_filename('/tmp/copied_output')
        assert(hashlib.sha1(open(SMALL_DATA_FILE, 'rb').read()).hexdigest() == hashlib.sha1(open('/tmp/copied_output', 'rb').read()).hexdigest())
        self.log.info('Copy successful!')

        # Cleanup
        self.log.info('Cleaning up put_copy test results (data, bucket, keys)...')
        os.system('rm /tmp/copied_output')
        for _k in _aux_b.list():
            _k.delete()
        time.sleep(7)
        _aux_b.delete()

    def test_multi_upload_to_completion(self):
        # Create multipart upload request
        _key_name = os.path.basename(self.source_path)
        mp = self.b.initiate_multipart_upload(_key_name)
        self.log.info('MPU initiated!')

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
        self.log.info('MPU Pieces Uploaded!')

        # Finish the upload
        mp.complete_upload()
        self.log.info('MPU Complete!')

        # Verify the upload
        k = Key(self.b)
        k.key = _key_name
        k.get_contents_to_filename(DATA_FILE+'_verify')
        diff_code = os.system('diff ' + DATA_FILE + ' ' + DATA_FILE + '_verify')
        if diff_code != 0:
            raise Exception('File corrupted!')

        # Cleanup
        os.system('rm ' + DATA_FILE + '_verify')

    def test_multi_upload_copy(self):
        # Create multipart upload request
        _key_name = os.path.basename(self.source_path)
        mp = self.b.initiate_multipart_upload(_key_name)
        self.log.info('MPU initiated!')

        # Upload a key to copy from
        self.log.info('Uploading test key...')
        _key = Key(self.b)
        _key.key = 'test_key'
        _key.set_contents_from_filename(SMALL_DATA_FILE)

        # Verify key is on S3
        _key_verify = Key(self.b)
        _key_verify.key = 'test_key'
        _key_verify.get_contents_to_filename('/tmp/test_verify')
        assert(hashlib.sha1(open('/tmp/test_verify', 'rb').read()).hexdigest() == hashlib.sha1(open(SMALL_DATA_FILE).read()).hexdigest())

        self.log.info('Copying keys into multipart file...')
        mp.copy_part_from_key(self.b, 'test_key', 1)
        mp.copy_part_from_key(self.b, 'test_key', 2)
        mp.copy_part_from_key(self.b, 'test_key', 3)
        mp.copy_part_from_key(self.b, 'test_key', 4)

        # Finish the upload
        self.log.info('Completing MP upload...')
        mp.complete_upload()
        self.log.info('MPU Complete!')

        self.log.info('Retrieving MP object...')
        k = Key(self.b)
        k.key = _key_name
        k.get_contents_to_filename(DATA_FILE+'_verify')

        # Create file for reference
        self.log.info('Creating ref object locally...')
        with open('/tmp/tmp_file_verify', 'wb') as tmp_file:
            with open(SMALL_DATA_FILE, 'rb') as data_file:
                for i in range(4):
                    tmp_file.write(data_file.read())
                    data_file.seek(0)

        # Verify
        self.log.info('Verifying...')
        assert(hashlib.sha1(open('/tmp/tmp_file_verify', 'rb').read()).hexdigest() == hashlib.sha1(open(DATA_FILE+'_verify').read()).hexdigest())

        # Cleanup
        os.system('rm ' + DATA_FILE + '_verify')
        os.system('rm ' + '/tmp/tmp_file_verify')

    def test_multi_upload_copy_multi_key(self):
        self.log.info('Uploading test keys')
        for i in range(4):
            _k = Key(self.b)
            _k.key = 'test_key' + str(i)
            _k.set_contents_from_filename(SMALL_DATA_FILE)

        # Verify keys
        self.log.info('Verifying keys')
        for i in range(4):
            _k = Key(self.b)
            _k.key = 'test_key' + str(i)
            _k.get_contents_to_filename('/tmp/test_verify' + str(i))
            assert(hashlib.sha1(open('/tmp/test_verify' + str(i), 'rb').read()).hexdigest() == hashlib.sha1(open(SMALL_DATA_FILE).read()).hexdigest())

        # Create MPU req
        _key_name = 'MPU_Key'
        mp = self.b.initiate_multipart_upload(_key_name)
        self.log.info('MPU Initiated!')
        self.log.info('Copying keys into MP file')

        for i in range(4):
            mp.copy_part_from_key(self.b.name, 'test_key' + str(i), i+1)

        # Finish the upload
        self.log.info('Completing MP upload...')
        mp.complete_upload()
        self.log.info('MPU Complete!')

        self.log.info('Retrieving MP object...')
        k = Key(self.b)
        k.key = _key_name
        k.get_contents_to_filename(DATA_FILE+'_verify')

        # Create reference object
        self.log.info('Creating ref object locally...')
        with open('/tmp/tmp_file_verify', 'wb') as tmp_file:
            with open(SMALL_DATA_FILE, 'rb') as data_file:
                for i in range(4):
                    tmp_file.write(data_file.read())
                    data_file.seek(0)

        # Verify
        self.log.info('Verifying...')
        assert(hashlib.sha1(open('/tmp/tmp_file_verify', 'rb').read()).hexdigest() == hashlib.sha1(open(DATA_FILE+'_verify').read()).hexdigest())

        # Cleanup
        os.system('rm ' + DATA_FILE + '_verify')
        for i in range(4):
            os.system('rm ' + '/tmp/test_verify' + str(i))

    def test_multi_upload_copy_same_key(self):
        # Upload a key to copy from
        self.log.info('Uploading test key...')
        _key = Key(self.b)
        _key.key = 'test_key'
        _key.set_contents_from_filename(SMALL_DATA_FILE)

        # Verify key is on S3
        _key_verify = Key(self.b)
        _key_verify.key = 'test_key'
        _key_verify.get_contents_to_filename('/tmp/test_verify')
        assert(hashlib.sha1(open('/tmp/test_verify', 'rb').read()).hexdigest() == hashlib.sha1(open(SMALL_DATA_FILE).read()).hexdigest())

        # Create multipart upload request
        _key_name = os.path.basename(self.source_path)
        mp = self.b.initiate_multipart_upload(_key_name)
        self.log.info('MPU initiated!')

        self.log.info('Copying keys into multipart file...')
        mp.copy_part_from_key(self.b.name, 'test_key', 1)
        mp.copy_part_from_key(self.b.name, 'test_key', 2)
        mp.copy_part_from_key(self.b.name, 'test_key', 3)
        mp.copy_part_from_key(self.b.name, 'test_key', 4)

        # Finish the upload
        self.log.info('Completing MP upload...')
        mp.complete_upload()
        self.log.info('MPU Complete!')

        self.log.info('Retrieving MP object...')
        k = Key(self.b)
        k.key = _key_name
        k.get_contents_to_filename(DATA_FILE+'_verify')

        # Create file for reference
        self.log.info('Creating ref object locally...')
        with open('/tmp/tmp_file_verify', 'wb') as tmp_file:
            with open(SMALL_DATA_FILE, 'rb') as data_file:
                for i in range(4):
                    tmp_file.write(data_file.read())
                    data_file.seek(0)

        # Verify
        self.log.info('Verifying...')
        assert(hashlib.sha1(open('/tmp/tmp_file_verify', 'rb').read()).hexdigest() == hashlib.sha1(open(DATA_FILE+'_verify').read()).hexdigest())

        # Cleanup
        os.system('rm ' + DATA_FILE + '_verify')
        os.system('rm ' + '/tmp/tmp_file_verify')

    def test_multi_upload_abort(self):
        # Create multipart upload request
        _key_name = os.path.basename(self.source_path)
        mp = self.b.initiate_multipart_upload(_key_name)
        self.log.info('MPU initiated!')

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

        self.b.cancel_multipart_upload(_key_name, mp.id)

        try:
            k = Key(self.b)
            k.key = _key_name
            k.get_contents_as_string()
        except: pass
        else:
            raise Exception('Found a key when it should not exist!')

    def test_multi_upload_list(self):
        # Create multipart upload request
        _key_name = os.path.basename(self.source_path)
        mp = self.b.initiate_multipart_upload(_key_name)
        self.log.info('MPU initiated!')

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
        self.log.info('MPU Pieces Uploaded!')

        for part in mp:
            self.log.info(str(part.part_number) + ' ' + str(part.size))
        self.log.info('Listing complete!')

    def tearDown(self):
        self.log.info('Beginning teardown...')
        for key in self.b.list():
            self.log.info('  Deleting keys...')
            key.delete()
        self.log.info('Attempting to delete bucket...')

        #TODO: This sleep is a workaround for WIN-913
        time.sleep(7)

        self.b.delete()
        self.log.info('Bucket successfully deleted!')
        self.log.info('Teardown complete!')

if __name__ == '__main__':
    global c

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', action='store_true', help='Enables verbose mode')
    parser.add_argument('--am_ip', default='127.0.0.1',
                        help='IP to run tests against')
    args = parser.parse_args()
    DEBUG = args.v
    HOST = args.am_ip

    # Make sure required files exist
    self.log.info('Data files dont exist, creating in /tmp')
    if not os.path.isfile(DATA_FILE):
        os.system('dd if=/dev/urandom of=' + DATA_FILE + ' bs=$(( 1024 * 1024 )) count=300')
    if not os.path.isfile(SMALL_DATA_FILE):
        os.system('dd if=/dev/urandom of=' + SMALL_DATA_FILE + ' bs=$(( 1024 * 1024 )) count=5')

    # Connect to FDS S3 interface
    c = boto.connect_s3(
        aws_access_key_id='blablabla',
        aws_secret_access_key='kekekekek',
        host=HOST,
        port=PORT,
        is_secure=False,
        calling_format=boto.s3.connection.OrdinaryCallingFormat())
    self.log.info('Connected!')

    suite = unittest.TestLoader().loadTestsFromTestCase(TestMultiUpload)
    unittest.TextTestRunner(verbosity=2).run(suite)

    c.close()
    self.log.info('All tests complete!')

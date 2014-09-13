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

def log(text, output=sys.stdout):
    if DEBUG:
        print >> output, text

class TestMultiUpload(unittest.TestCase):
    """
     Tests multiupload functionality on FDS
     Assumes that FDS is already running locally
    """
    def setUp(self):
        global c
        # Create bucket
        #TODO: Randomized bucket names are a temporary workaround for dodgy bucket delete on the server side.
        self._bucket_name = BUCKET_NAME + str(random.randint(0, 100000))
        self.b = c.create_bucket(self._bucket_name)
        time.sleep(2)
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

    # # Not supported for Goldman release
    # def test_list_bucket_objects_prefix(self):
    #     global c
    #     log('Uploading small object to %s' % self._bucket_name)
    #     k = Key(self.b)
    #     k.key = 'pref/test_key'
    #     k.set_contents_from_filename(SMALL_DATA_FILE)
    #
    #     log('Listing keys...')
    #     list = self.b.list(prefix='pref/')
    #     for key in list:
    #         log(key.key)

    def test_basic_head_object(self):
        global c
        log('Putting an object')
        _key = Key(self.b)
        _key.key = 'test_key'
        _key.set_contents_from_filename(SMALL_DATA_FILE)

        log('Checking for object with HEAD')
        _recv = self.b.get_key(_key.key)
        assert(_recv is not None)

    def test_list_keys(self):
        global c
        log('Putting 4 objects...')

        for i in range(4):
            _k = Key(self.b)
            _k.key = 'test_key' + str(i)
            _k.set_contents_from_filename(SMALL_DATA_FILE)

        # Verify keys
        log('Verifying keys')
        for i in range(4):
            _k = Key(self.b)
            _k.key = 'test_key' + str(i)
            _k.get_contents_to_filename('/tmp/test_verify' + str(i))
            assert(hashlib.sha1(open('/tmp/test_verify' + str(i), 'rb').read()).hexdigest() == hashlib.sha1(open(SMALL_DATA_FILE).read()).hexdigest())

        # List the keys
        for key in self.b.list():
            log(str(key))

    def test_put_copy(self):
        global c
        _aux_bucket_name = BUCKET_NAME + str(random.randint(0, 100000))
        log('Creating auxiliary bucket %s' % _aux_bucket_name)
        _aux_b = c.create_bucket(_aux_bucket_name)
        log('Bucket %s created!' % _aux_bucket_name)

        # Upload a key
        log('Uploading test key...')
        _key = Key(self.b)
        _key.key = 'putCopyTestObj'
        _key.set_contents_from_filename(SMALL_DATA_FILE)

        # Copy key
        log('Copying key to aux bucket')
        _aux_b.copy_key('copiedTestObj', self.b.name, _key.key)

        _copied_key = Key(_aux_b)
        _copied_key.key = 'copiedTestObj'
        _copied_key.get_contents_to_filename('/tmp/copied_output')
        assert(hashlib.sha1(open(SMALL_DATA_FILE, 'rb').read()).hexdigest() == hashlib.sha1(open('/tmp/copied_output', 'rb').read()).hexdigest())
        log('Copy successful!')

        # Cleanup
        log('Cleaning up put_copy test results (data, bucket, keys)...')
        os.system('rm /tmp/copied_output')
        for _k in _aux_b.list():
            _k.delete()
        time.sleep(7)
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

    def test_multi_upload_copy_multi_key(self):
        log('Uploading test keys')
        for i in range(4):
            _k = Key(self.b)
            _k.key = 'test_key' + str(i)
            _k.set_contents_from_filename(SMALL_DATA_FILE)

        # Verify keys
        log('Verifying keys')
        for i in range(4):
            _k = Key(self.b)
            _k.key = 'test_key' + str(i)
            _k.get_contents_to_filename('/tmp/test_verify' + str(i))
            assert(hashlib.sha1(open('/tmp/test_verify' + str(i), 'rb').read()).hexdigest() == hashlib.sha1(open(SMALL_DATA_FILE).read()).hexdigest())

        # Create MPU req
        _key_name = 'MPU_Key'
        mp = self.b.initiate_multipart_upload(_key_name)
        log('MPU Initiated!')
        log('Copying keys into MP file')

        for i in range(4):
            mp.copy_part_from_key(self.b, 'test_key' + str(i), i+1)

        # Finish the upload
        log('Completing MP upload...')
        mp.complete_upload()
        log('MPU Complete!')

        log('Retrieving MP object...')
        k = Key(self.b)
        k.key = _key_name
        k.get_contents_to_filename(DATA_FILE+'_verify')

        # Create reference object
        log('Creating ref object locally...')
        with open('/tmp/tmp_file_verify', 'wb') as tmp_file:
            with open(SMALL_DATA_FILE, 'rb') as data_file:
                for i in range(4):
                    tmp_file.write(data_file.read())
                    data_file.seek(0)

        # Verify
        log('Verifying...')
        assert(hashlib.sha1(open('/tmp/tmp_file_verify', 'rb').read()).hexdigest() == hashlib.sha1(open(DATA_FILE+'_verify').read()).hexdigest())

        # Cleanup
        os.system('rm ' + DATA_FILE + '_verify')
        for i in range(4):
            os.system('rm ' + '/tmp/test_verify' + str(i))

    def test_multi_upload_copy_same_key(self):
        # Upload a key to copy from
        log('Uploading test key...')
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
        log('MPU initiated!')

        log('Copying keys into multipart file...')
        mp.copy_part_from_key(self.b, 'test_key', 1)
        mp.copy_part_from_key(self.b, 'test_key', 2)
        mp.copy_part_from_key(self.b, 'test_key', 3)
        mp.copy_part_from_key(self.b, 'test_key', 4)

        # Finish the upload
        log('Completing MP upload...')
        mp.complete_upload()
        log('MPU Complete!')

        log('Retrieving MP object...')
        k = Key(self.b)
        k.key = _key_name
        k.get_contents_to_filename(DATA_FILE+'_verify')

        # Create file for reference
        log('Creating ref object locally...')
        with open('/tmp/tmp_file_verify', 'wb') as tmp_file:
            with open(SMALL_DATA_FILE, 'rb') as data_file:
                for i in range(4):
                    tmp_file.write(data_file.read())
                    data_file.seek(0)

        # Verify
        log('Verifying...')
        assert(hashlib.sha1(open('/tmp/tmp_file_verify', 'rb').read()).hexdigest() == hashlib.sha1(open(DATA_FILE+'_verify').read()).hexdigest())

        # Cleanup
        os.system('rm ' + DATA_FILE + '_verify')
        os.system('rm ' + '/tmp/tmp_file_verify')

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
        log('MPU Pieces Uploaded!')

        for part in mp:
            log(str(part.part_number) + ' ' + str(part.size))
        log('Listing complete!')

    def tearDown(self):
        log('Beginning teardown...')
        for key in self.b.list():
            log('  Deleting keys...')
            key.delete()
        log('Attempting to delete bucket...')

        #TODO: This sleep is a workaround for WIN-913
        time.sleep(7)

        self.b.delete()
        log('Bucket successfully deleted!')
        log('Teardown complete!')

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
    log('Data files dont exist, creating in /tmp')
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
    log('Connected!')

    suite = unittest.TestLoader().loadTestsFromTestCase(TestMultiUpload)
    unittest.TextTestRunner(verbosity=2).run(suite)

    c.close()
    log('All tests complete!')

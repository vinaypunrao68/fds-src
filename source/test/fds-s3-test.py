#!/usr/bin/python
from boto.exception import S3CreateError, S3ResponseError
import os
import sys
import math
import argparse
import random
import boto
import unittest
import time
import hashlib
import requests
from tempfile import NamedTemporaryFile
from boto.s3.connection import OrdinaryCallingFormat
from boto.s3.key import Key
from boto.s3.bucket import Bucket
from filechunkio import FileChunkIO
import pdb

random.seed(time.time())

def log(text, output=sys.stdout):
    if TEST_DEBUG:
        text = "\nFDS TEST: %s" % text
        print >> output, text

def get_new_name_helper(key_name_prefix):
    return key_name_prefix + "_" + str(random.randint(0, 100000))

def create_buckets_helper(conn, param, bucket_name=None, count=1):

    assert(count > 0)
    buckets_list = []
    for i in range(count):
        if bucket_name is None:
            bucket_name = get_new_name_helper(param.bucket_prefix)
        try:
            log("creating bucket %s" % bucket_name)
            if count == 1:
                name = bucket_name
            else:
                name = bucket_name + "_" + str(i)
            conn.create_bucket(name)
            buckets_list.append(name)
        except S3CreateError as e:
            log("Random bucket name should not exist %s" % bucket_name)
            log("FDS ERROR ({0}): {1}".format(e.error_code, e.message))
            raise
    return buckets_list


def create_keys_helper(conn, bucket_name, key_name_prefix, count, src_file_name, metadata=None):
    # TODO: Take in a list of src_file_name, iterate through that list as source data.
    assert(count > 0)
    key_names = []
    res_keys  = []
    for i in range(count):
        key_names.append(get_new_name_helper(key_name_prefix))

    bucket = conn.get_bucket(bucket_name)
    with open(src_file_name) as fp:
        for key_name in key_names:
            key = bucket.new_key(key_name)
            if metadata:
                key.update_metadata(metadata)
            key.set_contents_from_file(fp)
            res_keys.append(key)
            fp.seek(0)
    return res_keys

def delete_all_keys_helper(conn, bucket_name):
        bucket  = conn.get_bucket(bucket_name)
        res_set = bucket.get_all_keys()
        res_set = bucket.delete_keys(res_set)

def get_user_token(user, password, host, port, secure, validate):
    if secure:
        proto = 'https'
    else:
        proto = 'http'

    url = '%s://%s:%d/api/auth/token?login=%s&password=%s' % (proto, host, port, user, password)
    print("Getting credentials from: ", url)
    r = requests.get(url, verify=validate)
    rjson = r.json()
    print(rjson)
    return rjson['token']

class TestS3Conn(unittest.TestCase):

    # TODO: improve by creating/deleting more buckets
    def setUp(self):
        global g_connection
        self.conn  = g_connection.get_s3_connection()
        self.param = g_parameters
        self.param.bucket_name = get_new_name_helper(self.param.bucket_prefix)
        self.assertTrue(self.conn)

    def tearDown(self):
        self.assertTrue(self.conn)

    def test_create_bucket(self):
        create_buckets_helper(self.conn, self.param, self.param.bucket_name)

    def test_get_bucket(self):
        create_buckets_helper(self.conn, self.param, self.param.bucket_name)
        bucket = self.conn.get_bucket(self.param.bucket_name)
        self.assertTrue(bucket)

    def test_delete_bucket(self):
        bucket_name = create_buckets_helper(self.conn, self.param, count=1)

        # Cannot delete a bucket right after create
        time.sleep(5)
        self.conn.delete_bucket(bucket_name[0])
        # Sleep after delete before checking
        time.sleep(5)

    def test_get_all_buckets(self):
        create_buckets_helper(self.conn, self.param, self.param.bucket_name, count=1)
        buckets_list = self.conn.get_all_buckets()
        self.assertTrue(len(buckets_list) > 0)
        found = None
        for bucket in buckets_list:
            if self.param.verbose:
                log("bucket: %s - looking for %s" % (bucket.name, self.param.bucket_name))
            if self.param.bucket_name == bucket.name:
                found = True
        self.assertTrue(found)

    # last test to run, delete all buckets created
    def test_z_conn_delete_all_buckets(self):
        self.assertTrue(True)


class TestS3Bucket(unittest.TestCase):
    def setUp(self):
        global g_connection
        self.conn  = g_connection.get_s3_connection()
        self.param = g_parameters
        self.param.bucket_name = get_new_name_helper(self.param.bucket_prefix)
        self.assertTrue(self.conn)
        create_buckets_helper(self.conn, self.param, self.param.bucket_name, count=1)

    def tearDown(self):
        self.assertTrue(self.conn)

    # TODO: improve by using more keys and file_path.  Read files from a directory.
    def test_bucket_new_key(self):
        bucket = self.conn.get_bucket(self.param.bucket_name)
        key = bucket.new_key(self.param.key_name)
        with open(self.param.file_path) as fp:
            key.set_contents_from_file(fp)

    def test_bucket_list(self):
        bucket = self.conn.get_bucket(self.param.bucket_name)
        keys_list = bucket.list()
        found = False
        for k in keys_list:
            log("key: %s" % k.name)
            if k.name == self.param.key_name:
                log("found key listing: %s" % k.name)
                found = True
        self.assertTrue(found)

    def test_bucket_copy_key(self):
        bucket = self.conn.get_bucket(self.param.bucket_name)
        key = bucket.new_key(self.param.key_name)
        with open(self.param.file_path) as fp:
            key.set_contents_from_file(fp)

        new_metadata = { "meta1" : "meta value 1" }

        # XXX: metadata below will not be copy!
        new_key = bucket.copy_key(get_new_name_helper(key.name),
                                  self.param.bucket_name,
                                  key.name,
                                  metadata=new_metadata)

        new_key.update_metadata(new_metadata)
        new_key.set_metadata("meta2", "meta value 2")
        with open(self.param.file_path) as fp:
            new_key.set_contents_from_file(fp)

        # Do some checking here
    # Delete the bucket
    def test_bucket_delete(self):
        bucket_name = create_buckets_helper(self.conn, self.param, count=1)
        log("Creating and deleting %s" % bucket_name[0])

        # Cannot delete a bucket right after create
        time.sleep(5)
        bucket = self.conn.get_bucket(bucket_name[0])
        bucket.delete()
        # Need to wait a bit after deleting the bucket before moving on
        time.sleep(5)

        # TODO: Add a case for delete non-empty volume.  Not now, FDS does not handle this.


    def test_bucket_delete_key(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 1, self.param.file_path)
        self.assertTrue(len(res_keys) == 1)

        bucket  = self.conn.get_bucket(self.param.bucket_name)
        key     = res_keys[0]
        res_key = bucket.get_key(key.name)
        self.assertTrue(res_key is not None)
        self.assertTrue(res_key.name == key.name)

        bucket.delete_key(key.name)
        res_key = bucket.get_key(key.name)
        self.assertTrue(res_key is None)

    def test_bucket_delete_keys(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 6, self.param.file_path)
        self.assertTrue(len(res_keys) == 6)

        bucket = self.conn.get_bucket(self.param.bucket_name)
        res_list = bucket.delete_keys(res_keys)
        for res in res_list.deleted:
            found = 0
            for key in res_keys:
                if res.key == key.name:
                    found = found + 1
            self.assertTrue(found == 1)
        self.assertTrue(len(res_list.errors) == 0)

    def test_bucket_get_all_keys(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 6, self.param.file_path)
        self.assertTrue(len(res_keys) == 6)
        bucket = self.conn.get_bucket(self.param.bucket_name)
        res_set = bucket.get_all_keys()

        found = 0
        for res in res_set:
            for key in res_keys:
                if res.name == key.name:
                    found = found + 1
        self.assertTrue(found == len(res_keys))

    def test_bucket_list(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 6, self.param.file_path)
        self.assertTrue(len(res_keys) == 6)
        bucket = self.conn.get_bucket(self.param.bucket_name)

        res_set = bucket.list()
        found = 0
        for res in res_set:
            for key in res_keys:
                if res.name == key.name:
                    found = found + 1
        self.assertTrue(found == len(res_keys))

    # Last test to run, cleanup test bucket.
    def test_z_delete_all_keys(self):
        delete_all_keys_helper(self.conn, self.param.bucket_name)

class TestS3Key(unittest.TestCase):
    def setUp(self):
        global g_connection
        self.conn  = g_connection.get_s3_connection()
        self.param = g_parameters
        self.param.bucket_name = get_new_name_helper(self.param.bucket_prefix)
        self.assertTrue(self.conn)
        create_buckets_helper(self.conn, self.param, self.param.bucket_name, count=1)

    def tearDown(self):
        self.assertTrue(self.conn)

    def key_verify_data_content(self, key1, key2):
        # verify data + metadata
        data1 = key1.get_contents_as_string()
        data2 = key2.get_contents_as_string()
        hash1 = hashlib.sha1(data1).hexdigest()
        hash2 = hashlib.sha1(data2).hexdigest()
        self.assertTrue(hash1 == hash2)

        self.assertTrue(key1.metadata == key2.metadata)
        self.assertTrue(key1.cache_control == key2.cache_control)
        self.assertTrue(key1.content_type == key2.content_type)
        self.assertTrue(key1.content_encoding == key2.content_encoding)
        self.assertTrue(key1.content_disposition == key2.content_disposition)
        self.assertTrue(key1.content_language == key2.content_language)
        self.assertTrue(key1.etag == key2.etag)
        self.assertTrue(key1.owner == key2.owner)
        self.assertTrue(key1.storage_class == key2.storage_class)
        self.assertTrue(key1.md5 == key2.md5)
        self.assertTrue(key1.size == key2.size)
        self.assertTrue(key1.encrypted == key2.encrypted)

    def test_key_copy(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 1, self.param.file_path)
        self.assertTrue(len(res_keys) == 1)
        bucket = self.conn.get_bucket(self.param.bucket_name)
        res_key = bucket.get_key(res_keys[0].name)
        self.assertTrue(res_key is not None)

        # copy to new key
        new_key_name = get_new_name_helper(self.param.key_name)
        res_key.copy(bucket.name, new_key_name, preserve_acl=True)
        new_key = bucket.get_key(new_key_name)

        # verify data + metadata
        self.key_verify_data_content(res_key, new_key)

        # overwriting key's metadata
        new_metadata = { 'test_meta1' : 'test_meta_value1',
                         'test_meta2' : 'test_meta_value2'
        }
        res_key.copy(bucket.name, res_key.name, metadata=new_metadata, preserve_acl=True)
        res_key = bucket.get_key(res_key.name)
        self.assertTrue(res_key.metadata == new_metadata)

    def test_key_delete(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)
        bucket = self.conn.get_bucket(self.param.bucket_name)
        for key in res_keys:
            key.delete()
            res_key = bucket.get_key(key.name)
            self.assertTrue(res_key is None)

    def test_key_exists(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)
        for key in res_keys:
            self.assertTrue(key.exists())

    def test_key_get_contents_as_string(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        with open(self.param.file_path) as data_fp:
            data_str = data_fp.read()

        for key in res_keys:
            str = key.get_contents_as_string()
            self.assertEqual(hashlib.sha1(str).hexdigest(),
                             hashlib.sha1(data_str).hexdigest())

    def test_key_get_contents_to_file(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        with open(self.param.file_path) as data_fp:
            data_str = data_fp.read()

        for key in res_keys:
            tmp_file = get_new_name_helper("/tmp/boto_test_file")
            with open(tmp_file, 'w+') as fp:
                key.get_contents_to_file(fp)

            with open(tmp_file, 'r') as fp:
                tmp_str = fp.read()
                self.assertEqual(hashlib.sha1(tmp_str).hexdigest(),
                                 hashlib.sha1(data_str).hexdigest())

                # don't remove the file if the check fails.
                os.system('rm ' + tmp_file)


    def test_key_get_contents_to_filename(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        with open(self.param.file_path) as data_fp:
            data_str = data_fp.read()

        for key in res_keys:
            tmp_file = get_new_name_helper("/tmp/boto_test_file")
            key.get_contents_to_filename(tmp_file)

            with open(tmp_file, 'r') as fp:
                tmp_str = fp.read()
                self.assertEqual(hashlib.sha1(tmp_str).hexdigest(),
                                 hashlib.sha1(data_str).hexdigest())

                # don't remove the file if the check fails.
                os.system('rm ' + tmp_file)

    def test_key_get_file(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        with open(self.param.file_path) as data_fp:
            data_str = data_fp.read()

        for key in res_keys:
            tmp_file = get_new_name_helper("/tmp/boto_test_file")
            with open(tmp_file, 'w+') as fp:
                key.get_file(fp)

            with open(tmp_file, 'r') as fp:
                tmp_str = fp.read()
                self.assertEqual(hashlib.sha1(tmp_str).hexdigest(),
                                 hashlib.sha1(data_str).hexdigest())

                # don't remove the file if the check fails.
                os.system('rm ' + tmp_file)

    def test_key_get_metadata(self):
        md = { 'test_meta1' : 'test_value1',
               'test_meta2' : 'test_value2'
        }
        res_keys = create_keys_helper(self.conn,
                                      self.param.bucket_name,
                                      self.param.key_name,
                                      10,
                                      self.param.file_path,
                                      metadata=md)
        self.assertTrue(len(res_keys) == 10)

        for key in res_keys:
            md_val = key.get_metadata('test_meta1')
            self.assertTrue(md_val == 'test_value1')
            md_val = key.get_metadata('test_meta2')
            self.assertTrue(md_val == 'test_value2')

    def failed_test_key_send_file(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        for key in res_keys:
            f = NamedTemporaryFile(delete=False)
            f.write(bytearray(os.urandom(100)))
            f.seek(0)

            with open(f.name) as fp:
                # md5 and length has to be the same
                data = fp.read()
                md5 = hashlib.md5(data).hexdigest()
                key.md5 = md5
                fp.seek(0)
                hdrs = { "content_MD5" : md5,
                         "content_length" : str(len(data))
                }
                try:
                    key.send_file(fp, headers=hdrs, size=len(data))
                except S3ResponseError as e:
                    pdb.set_trace()
                    raise e

            f.close()


    def get_verify_remote_content_str(self, key, tmp_str):
        res_str = key.get_contents_as_string()
        self.assertTrue(res_str == tmp_str)

    def get_verify_remote_content(self, key, tmp_f):
        self.get_verify_remote_content_str(key, tmp_f.read())

    def test_key_set_contents_from_file(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        for key in res_keys:
            f = NamedTemporaryFile()
            f.write(bytearray(os.urandom(4096)))
            f.seek(0)

            with open(f.name) as fp:
                key.set_contents_from_file(fp)

            f.seek(0)
            self.get_verify_remote_content(key, f)
            f.close()

    def test_key_set_contents_from_filename(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        for key in res_keys:
            f = NamedTemporaryFile()
            f.write(bytearray(os.urandom(4096)))
            f.seek(0)

            key.set_contents_from_filename(f.name)

            f.seek(0)
            self.get_verify_remote_content(key, f)
            f.close()

    def failed_test_key_set_contents_from_stream(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        for key in res_keys:
            f = NamedTemporaryFile(delete=False)
            f.write(bytearray(os.urandom(1024)))
            f.seek(0)

            with open(f.name) as fp:
                # md5 and length has to be the same
                key.set_contents_from_stream(fp)

            f.close()

    def test_key_set_content_from_string(self):
        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        for key in res_keys:
            tmp_str = str(bytearray(os.urandom(4096)))
            key.set_contents_from_string(tmp_str)
            self.get_verify_remote_content_str(key, tmp_str)

    def test_key_set_metadata(self):
        md1 = { 'new_meta' : 'new_meta_value' }

        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        bucket = self.conn.get_bucket(self.param.bucket_name)
        for key in res_keys:
            key.set_metadata('new_meta', 'new_meta_value')
            self.assertTrue(key.metadata == md1)

    def test_key_set_remote_metadata(self):
        md1 = { 'meta1' : 'value1',
                'meta2' : 'value2'
        }
        md2 = { 'meta3' : 'value3',
                'meta4' : 'value4'
        }

        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        bucket = self.conn.get_bucket(self.param.bucket_name)
        for key in res_keys:
            key.set_remote_metadata(md1, {}, preserve_acl=True)
            res = bucket.get_key(key.name)
            self.assertTrue(res.metadata == md1)

        for key in res_keys:
            key.set_remote_metadata(md2, md1, preserve_acl=True)
            res = bucket.get_key(key.name)
            self.assertTrue(res.metadata == md2)

    def test_key_update_metadata(self):
        md1 = { 'meta1' : 'value1',
                'meta2' : 'value2'
        }

        res_keys = create_keys_helper(self.conn, self.param.bucket_name, self.param.key_name, 10, self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        bucket = self.conn.get_bucket(self.param.bucket_name)
        for key in res_keys:
            key.update_metadata(md1)
            self.assertTrue(key.metadata == md1)

    def test_z_delete_all_keys(self):
        delete_all_keys_helper(self.conn, self.param.bucket_name)

class TestParameters():
    def __init__(self, bucket_name, bucket_prefix, key_name, keys_count, file_path, verbose):
        self.bucket_name = bucket_name
        self.bucket_prefix = bucket_prefix
        self.key_name    = key_name
        self.file_path   = file_path
        self.verbose     = verbose
        self.keys_count  = keys_count

class S3Connection():
    def __init__(self, id, token, host, port, secure, debug=0):
        self.access_key_id      = id
        self.secret_access_key  = token
        self.host               = host
        self.port               = port
        self.is_secure          = secure
        self.debug              = debug
        self.connection         = None

    def s3_connect(self):
        assert(self.connection is None)
        self.connection = boto.connect_s3(aws_access_key_id=self.access_key_id,
                                          aws_secret_access_key=self.secret_access_key,
                                          host=self.host,
                                          port=self.port,
                                          is_secure=self.is_secure,
                                          calling_format=boto.s3.connection.OrdinaryCallingFormat(),
                                          debug=self.debug)
        assert(self.connection)
        log('S3 Connected!')

    def s3_disconnect(self):
        assert(self.connection)
        self.connection.close()
        log('S3 Disconnected')

    def get_s3_connection(self):
        return self.connection


# global declaration
#FDS_DEFAULT_KEY_ID            = 'demo_user'
#FDS_DEFAULT_SECRET_ACCESS_KEY = 'a9658f9c5b1ba032f444cb7847aa7f7fce7629821fdfc7078f32429d4a343730a5492dce2431d2af2b4f2e8cac222395d87006fc4e21cceab5418cba5a2cdd06'
#FDS_DEFAULT__HOST             = 'us-east.formationds.com'

FDS_DEFAULT_KEY_ID            = 'AKIAJCNNNWKKBQU667CQ'
FDS_DEFAULT_SECRET_ACCESS_KEY = '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'
FDS_DEFAULT__HOST             = 's3.amazonaws.com'

FDS_DEFAULT_PORT              = 443
FDS_AUTH_DEFAULT_PORT         = 443
FDS_DEFAULT_IS_SECURE         = True

FDS_DEFAULT_BUCKET_NAME       = "demo_volume2"
FDS_DEFAULT_BUCKET_PREFIX     = "demo_volume"
FDS_DEFAULT_KEY_NAME          = "file_1"
FDS_DEFAULT_KEYS_COUNT        = 10
FDS_DEFAULT_FILE_PATH         = "/tmp/foobar"
TEST_DEBUG                    = 1

def main():
    global g_connection
    global g_parameters

    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', action='store_true', default=False,
                        help='enables verbose mode')
    parser.add_argument('--debug', default=0, dest='debug', type=int,
                        help='Set boto debug level [1,2]')
    parser.add_argument('--am_url', default=FDS_DEFAULT__HOST,
                        help='ip/url to run tests against')
    parser.add_argument('--am_port', default=FDS_DEFAULT_PORT, type=int,
                        help='port to run tests against')
    parser.add_argument('--user_id', default=FDS_DEFAULT_KEY_ID,
                        help='access key id/user')
    parser.add_argument('--user_token', default=FDS_DEFAULT_SECRET_ACCESS_KEY,
                        help='secret key id')
    parser.add_argument('--insecure', action='store_false', dest="secure",
                        default=FDS_DEFAULT_IS_SECURE,
                        help='Disable secure (SSL) mode')
    parser.add_argument('--secure', action='store_true', dest="secure",
                        help='Enable secure (SSL) mode - default on')
    parser.add_argument('--fds_pass', dest="fds_pass", default=False,
                        help='define FDS pass to use Tenant API for auth token')
    parser.add_argument('--auth_port', dest="auth_port",
                        default=FDS_AUTH_DEFAULT_PORT, type=int,
                        help='define FDS pass to use Tenant API for auth token')
    parser.add_argument('--novalidate-cert', action='store_false', default=True,
                        dest='validate',
                        help='Disable cert validation in URL requests')

    args         = parser.parse_args()
    TEST_DEBUG   = args.verbose
    # If we pass in --fds_pass then we use the API to get our token
    if args.fds_pass != False:
        args.user_token = get_user_token(user=args.user_id,
                                         password=args.fds_pass,
                                         host=args.am_url,
                                         port=args.auth_port,
                                         secure=args.secure,
                                         validate=args.validate)
    g_connection = S3Connection(args.user_id,
                                args.user_token,
                                args.am_url,
                                args.am_port,
                                args.secure,
                                args.debug)

    g_parameters = TestParameters(FDS_DEFAULT_BUCKET_NAME,
                                  FDS_DEFAULT_BUCKET_PREFIX,
                                  FDS_DEFAULT_KEY_NAME,
                                  FDS_DEFAULT_KEYS_COUNT,
                                  FDS_DEFAULT_FILE_PATH,
                                  TEST_DEBUG)

    g_connection.s3_connect()


    test_classes = [TestS3Conn, TestS3Bucket, TestS3Key]
    loader       = unittest.TestLoader()
    suites_list  = []

    # Make sure FDS_DEFAULT_FILE_PATH exists
    if not os.path.isfile(FDS_DEFAULT_FILE_PATH):
        file = open(FDS_DEFAULT_FILE_PATH, 'w+')
        file.write('This file used for testing fds-s3-test.py')
        file.close()

    for test_class in test_classes:
        suite = loader.loadTestsFromTestCase(test_class)
        suites_list.append(suite)

    all_suites = unittest.TestSuite(suites_list)
    runner     = unittest.TextTestRunner(verbosity=2)
    results    = runner.run(all_suites)

    g_connection.s3_disconnect()
    log('All tests complete!')

if __name__ == '__main__':
    main()

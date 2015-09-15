
import random
import time

import s3
import lib
import testsets.testcase as testcase

class TestS3Conn(testcase.FDSTestCase):

    # TODO: improve by creating/deleting more buckets
    def __init__(self, parameters=None):
        super(TestS3Conn, self).__init__(parameters)
        self.conn = self.s3conn.get_s3_connection()
        self.param.bucket_name = lib.get_new_name_helper(self.param.bucket_prefix)

    def tearDown(self):
        pass

    def test_create_bucket(self):
        lib.create_buckets_helper(self.conn, self.param,
                                    self.param.bucket_name)

    def test_get_bucket(self):
        lib.create_buckets_helper(self.conn, self.param,
                                    self.param.bucket_name)
        bucket = self.conn.get_bucket(self.param.bucket_name)
        self.assertTrue(bucket)

    def test_delete_bucket(self):
        bucket_name = lib.create_buckets_helper(self.conn, self.param,
                                                  count=1)

        # Cannot delete a bucket right after create
        time.sleep(5)
        self.conn.delete_bucket(bucket_name[0])
        # Sleep after delete before checking
        time.sleep(5)

    def test_get_all_buckets(self):
        lib.create_buckets_helper(self.conn, self.param,
                                    self.param.bucket_name,
                                    count=1)
        buckets_list = self.conn.get_all_buckets()
        self.assertTrue(len(buckets_list) > 0)
        found = None
        for bucket in buckets_list:
            if self.param.verbose:
                self.log.info("bucket: %s - looking for %s" % (bucket.name,
                                                               self.param.bucket_name))
            if self.param.bucket_name == bucket.name:
                found = True
        self.assertTrue(found)

    # last test to run, delete all buckets created
    def test_z_conn_delete_all_buckets(self):
        self.assertTrue(True)

class TestS3Bucket(testcase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3Bucket, self).__init__(parameters)
        self.conn = self.s3conn.get_s3_connection()
        self.log.info(self.conn)
        self.param.bucket_name = lib.get_new_name_helper(self.param.bucket_prefix)
        lib.create_buckets_helper(self.conn, self.param,
                                    self.param.bucket_name, count=1)

    def tearDown(self):
        pass

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
        bucket_name = lib.create_buckets_helper(self.conn, self.param,
                                                  count=1)
        self.log.info("Creating and deleting %s" % bucket_name[0])

        # Cannot delete a bucket right after create
        time.sleep(5)
        bucket = self.conn.get_bucket(bucket_name[0])
        bucket.delete()
        # Need to wait a bit after deleting the bucket before moving on
        time.sleep(5)

        # TODO: Add a case for delete non-empty volume.  Not now, FDS does not handle this.


    def test_bucket_delete_key(self):
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 1,
                                            self.param.file_path)
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
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 6,
                                            self.param.file_path)
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
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 6,
                                            self.param.file_path)
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
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 6,
                                            self.param.file_path)
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
        lib.delete_all_keys_helper(self.conn, self.param.bucket_name)
        
class TestS3Key(testcase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3Key, self).__init__(parameters)
        self.conn = self.s3conn.get_s3_connection()
        self.param.bucket_name = lib.get_new_name_helper(self.param.bucket_prefix)
        lib.create_buckets_helper(self.conn, self.param,
                                    self.param.bucket_name, count=1)

    def tearDown(self):
        pass

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
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 1,
                                            self.param.file_path)
        self.assertTrue(len(res_keys) == 1)
        bucket = self.conn.get_bucket(self.param.bucket_name)
        res_key = bucket.get_key(res_keys[0].name)
        self.assertTrue(res_key is not None)

        # copy to new key
        new_key_name = lib.get_new_name_helper(self.param.key_name)
        res_key.copy(bucket.name, new_key_name, preserve_acl=True)
        new_key = bucket.get_key(new_key_name)

        # verify data + metadata
        self.key_verify_data_content(res_key, new_key)

        # overwriting key's metadata
        new_metadata = { 'test_meta1' : 'test_meta_value1',
                         'test_meta2' : 'test_meta_value2'
        }
        res_key.copy(bucket.name, res_key.name, metadata=new_metadata,
                     preserve_acl=True)
        res_key = bucket.get_key(res_key.name)
        self.assertTrue(res_key.metadata == new_metadata)

    def test_key_delete(self):
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
        self.assertTrue(len(res_keys) == 10)
        bucket = self.conn.get_bucket(self.param.bucket_name)
        for key in res_keys:
            key.delete()
            res_key = bucket.get_key(key.name)
            self.assertTrue(res_key is None)

    def test_key_exists(self):
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
        self.assertTrue(len(res_keys) == 10)
        for key in res_keys:
            self.assertTrue(key.exists())

    def test_key_get_contents_as_string(self):
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        with open(self.param.file_path) as data_fp:
            data_str = data_fp.read()

        for key in res_keys:
            str = key.get_contents_as_string()
            self.assertEqual(hashlib.sha1(str).hexdigest(),
                             hashlib.sha1(data_str).hexdigest())

    def test_key_get_contents_to_file(self):
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        with open(self.param.file_path) as data_fp:
            data_str = data_fp.read()

        for key in res_keys:
            tmp_file = lib.get_new_name_helper("/tmp/boto_test_file")
            with open(tmp_file, 'w+') as fp:
                key.get_contents_to_file(fp)

            with open(tmp_file, 'r') as fp:
                tmp_str = fp.read()
                self.assertEqual(hashlib.sha1(tmp_str).hexdigest(),
                                 hashlib.sha1(data_str).hexdigest())

                # don't remove the file if the check fails.
                os.system('rm ' + tmp_file)


    def test_key_get_contents_to_filename(self):
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        with open(self.param.file_path) as data_fp:
            data_str = data_fp.read()

        for key in res_keys:
            tmp_file = lib.get_new_name_helper("/tmp/boto_test_file")
            key.get_contents_to_filename(tmp_file)

            with open(tmp_file, 'r') as fp:
                tmp_str = fp.read()
                self.assertEqual(hashlib.sha1(tmp_str).hexdigest(),
                                 hashlib.sha1(data_str).hexdigest())

                # don't remove the file if the check fails.
                os.system('rm ' + tmp_file)

    def test_key_get_file(self):
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        with open(self.param.file_path) as data_fp:
            data_str = data_fp.read()

        for key in res_keys:
            tmp_file = lib.get_new_name_helper("/tmp/boto_test_file")
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
        res_keys = lib.create_keys_helper(self.conn,
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
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
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
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
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
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
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
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
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
        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        for key in res_keys:
            tmp_str = str(bytearray(os.urandom(4096)))
            key.set_contents_from_string(tmp_str)
            self.get_verify_remote_content_str(key, tmp_str)

    def test_key_set_metadata(self):
        md1 = { 'new_meta' : 'new_meta_value' }

        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
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

        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
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

        res_keys = lib.create_keys_helper(self.conn, self.param.bucket_name,
                                            self.param.key_name, 10,
                                            self.param.file_path)
        self.assertTrue(len(res_keys) == 10)

        bucket = self.conn.get_bucket(self.param.bucket_name)
        for key in res_keys:
            key.update_metadata(md1)
            self.assertTrue(key.metadata == md1)

    def test_z_delete_all_keys(self):
        delete_all_keys_helper(self.conn, self.param.bucket_name)
import boto
import contextlib
import functools
import glob
import logging
import math
import multiprocessing
import os
import random
import subprocess
import shutil
import sys

from boto.s3.key import Key
from filechunkio import FileChunkIO
from multiprocessing.pool import IMapIterator

import config
import s3
import samples
import users
import lib

class S3Volumes(object):

    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)
    '''
    This API defines all the operations to be performed using a S3 bucket for
    FDS, including downloading, uploading, creating and deleting buckets.

    Attributes:
    -----------
    name: the name pattern to be given to the bucket
    om_ip_address: the IP address to where the OM node is located.
    am_ip_address: the IP address to where the AM node is located. If not
    specified, the AM node will be assumed to the the same as the OM node
    '''
    def __init__(self, om_ip_address, am_ip_address=None):
        self.om_ip_address = om_ip_address
        # in the case the AM IP Address is not specified, then use the
        # OM IP address as the AM IP Address
        if am_ip_address is None:
            self.am_ip_address = om_ip_address
        else:
            self.am_ip_address = am_ip_address
        self.s3conn = lib.create_s3_connection(self.om_ip_address,
                                                 self.am_ip_address)
        self.buckets = []

    def create_volumes(self, quantity, name):
        '''
        Given number specified by the user, create that many buckets specified.

        Attributes:
        -----------
        size: the number of buckets to be created
        '''
        assert quantity > 0
        # connect to the S3 instance
        for i in xrange(0, quantity):
            bucket_name = "%s_%s" % (name, i)
            bucket = self.__create_volume(self.s3conn, bucket_name)
            self.buckets.append(bucket)

        return self.buckets

    def __create_volume(self, s3conn, bucket_name):
        '''
        For each of the AM instances, create a single bucket for every volume,
        and populate that volume with sample data.

        Attributes:
        -----------
        s3conn: S3Connection
            a S3Connection to the host machine
        bucket_name:
            the name of the bucket being created

        Returns:
        --------
        bucket : The S3 bucket object
        '''
        bucket = s3conn.conn.create_bucket(bucket_name)
        if bucket == None:
            raise Exception("Invalid bucket.")
            # We won't be waiting for it to complete, short circuit it.
            self.test_passed = False
            self.reportTestCaseResult(self.test_passed)

        self.log.info("Volume %s created..." % bucket.name)
        return bucket

    def get_all_buckets(self):
        '''
        Return a list of all the buckets in S3
        '''
        return self.s3conn.conn.get_all_buckets()

    def get_bucket(self, volume_name):
        return self.s3conn.conn.get_bucket(volume_name)

    def download_files(self, bucket, dest):
        '''
        Download all the files present in a S3 volume

        Attributes:
        -----------
        bucket: The s3 volume (bucket) to where download the data from
        '''
        bucket_list = bucket.list()
        for l in bucket_list:
            key_string = str(l.key)
            path = os.path.join(dest, key_string)
            # check if file exists locally, if not: download it
            if not os.path.exists(path):
                self.log.info("Downloading %s" % path)
                l.get_contents_to_filename(path)

    def store_file_to_volume(self, bucket, filepath, key_name):
        '''
        Given the list of files to be uploaded, presented in sample_files list,
        upload them to the corresponding volume

        Attributes:
        -----------
        bucket : bucket
            the S3 bucket (volume) where the data files will to uploaded to.
        '''
        # add the data files to the bucket.
        k = None
        try:
            k = bucket.get_key(key_name)
        except:
            pass
        if k is None:
            k = bucket.new_key(key_name)
        if os.path.exists(filepath):
            #print("key is {}".format(k.key))
            k.set_contents_from_filename(filepath,
                                         cb=lib.percent_cb,
                                         num_cb=10)
            self.log.info("Uploaded file %s to bucket %s" % 
                         (filepath, bucket.name))

    def store_multipart_uploads(self, bucket, source_path, chunk_size=52428800):
        '''
        Given a large file (size >= 200MB), we want to perform a multipart
        upload. This method breaks a file into pieces of chunk_size (default 50MB)
        and perform a multipart upload in each of those pieces. This allows us
        to upload a large file much faster.
        
        Attributes:
        ===========
        bucket: a bucket object
        source_path: full path to where is the file to be uploaded is located.
        chunk_size: The size of the partition. Defaults to 50MB
        '''
        if not os.path.exists(source_path):
            raise IOError("File %s doesn't exist or full path not given")
            sys.exit(2)

        source_size = os.stat(source_path).st_size
        multipart = bucket.initiate_multipart_upload(os.path.basename(source_path))
        assert chunk_size != 0
        chunk_count = int(math.ceil(source_size / float(chunk_size)))
        self.log.info("Multipart uploading: %s" % source_path)
        # send the file parts, using FileChunkIO to create a file-like object
        # that points to a certain byte range within the original file. We
        # set bytes to never exceed the original file size.
        for i in xrange(0, chunk_count):
            offset = chunk_size*i
            bytes = min(chunk_size, source_size - offset)
            with FileChunkIO(source_path, 'r', offset=offset, bytes=bytes) as fp:
                multipart.upload_part_from_file(fp, part_num=i+1)
                
        # finish the upload
        multipart.complete_upload()

    @contextlib.contextmanager
    def multimap(self, cores=None):
        '''
        Provide multiprocessing imap like function.
        
        The context manager handles setting up the pool, worked around interrupt
        issues and terminating the pool on completion.
        '''
        if cores is None:
            cores = max(multiprocessing.cpu_count() -1 , 1)
        def wrapper(func):
            def wrap(self, timeout=None):
                return func(self, timeout=timeout if timeout is not None else 1e100)
            return wrap
        IMapIterator.next = wrapper(IMapIterator.next)
        pool = multiprocessing.Pool(cores)
        yield pool.imap
        pool.terminate()

    def upload_cb(self, complete, total):
        sys.stdout.write('.')
        sys.stdout.flush()
    
    def map_wrap(self, f):
        @functools.wraps(f)
        def wrapper(*args, **kwargs):
            return apply(f, *args, **kwargs)
        return wrapper
    
    def mp_from_ids(self, mp_id, mp_keyname, mp_bucketname):
        '''
        Get the multipart upload from the bucket and multipart IDs.
        This allows us to reconstitute a connection to the upload from within
        multiprocessing functions. 
        '''
        bucket = self.s3conn.conn.lookup(mp_bucketname)
        mp = boto.s3.multipart.MultiPartUpload(bucket)
        mp.key_name = mp_keyname
        mp.id = mp_id
        return mp
    
    def transfer_part(self, mp_id, mp_keyname, mp_bucketname, i, part):
        '''
        Transfer a part of the multipart upload. Designed to be run in parallel.
        '''
        mp = self.mp_from_ids(mp_id, mp_keyname, mp_bucketname)
        self.log.info("Transferring %s %s" % (i, part))
        with open(part) as t_handle:
            mp.upload_part_from_file(t_handle, i+1)
        os.remove(part)
    
    def multipart_upload(self, bucket, s3_key_name, tarball, mb_size, use_rr=True):
        '''
        Upload large files using Amazon's multipart upload functionality.
        '''
        cores = multiprocessing.cpu_count()
        def split_file(in_file, mb_size, split_num=5):
            prefix = os.path.join(os.path.dirname(in_file),
                                  "%sS3PART" % (os.path.basename(s3_key_name)))
            split_size = int(min(mb_size / (split_num * 2.0), 250))
            if not os.path.exists(prefix):
                cl = ["split", "-b%sm" % split_size, in_file, prefix]
                subprocess.check_call(cl)
            return sorted(glob.glob("%s*" % prefix))
            
        mp = bucket.initiate_multipart_upload(os.path.basename(source_path),
                                              reduced_redundancy=use_rr)
        with multimap(cores) as pmap:
            for _ in pmap(self.transfer_part, ((mp.id, mp.key_name, mp.bucket_name, i, part)
                                               for (i, part) in enumarate(split_file(source_path,
                                                                          mb_size, cores)))):
                pass
        mp.complete_upload()

    def delete_volumes(self, buckets):
        '''
        Given a list of volumes to be deleted, remove all of them

        Attributes:
        ----------
        buckets: a list of S3 buckets
        '''
        for bucket in buckets:
            self.__delete_volume(self.s3conn, bucket)
        # After the clean up please delete the bucket
        self.s3conn.s3_disconnect()

    def __delete_volume(self, s3conn, bucket):
        '''
        After test execute, remove the current bucket associated with this
        connection

        Attributes:
        -----------
        s3conn : S3Conn
            the connection object to the FDS instance via S3
        bucket: S3
            the bucket associated with this connection
        '''
        if bucket:
            for key in bucket.list():
                bucket.delete_key(key)
            self.log.info("Deleting bucket: %s", bucket.name)
            self.s3conn.conn.delete_bucket(bucket.name)

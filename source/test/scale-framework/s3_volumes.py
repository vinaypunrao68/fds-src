import boto
import os
import random
import logging
import shutil
import sys

from boto.s3.key import Key

import config
import s3
import samples
import users
import utils

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
    def __init__(self, name, om_ip_address, am_ip_address=None):
        self.name = name
        self.om_ip_address = om_ip_address
        # in the case the AM IP Address is not specified, then use the
        # OM IP address as the AM IP Address
        if am_ip_address is None:
            self.am_ip_address = om_ip_address
        else:
            self.am_ip_address = am_ip_address
        self.s3conn = utils.create_s3_connection(self.om_ip_address,
                                                 self.am_ip_address)
        self.buckets = []

    def create_volumes(self, size):
        '''
        Given number specified by the user, create that many buckets specified.

        Attributes:
        -----------
        size: the number of buckets to be created
        '''
        assert size > 0
        # connect to the S3 instance
        for i in xrange(0, size):
            bucket_name = "%s_%s" % (self.name, i)
            bucket = self.__create_volume(self.s3conn, bucket_name)
            self.buckets.append(bucket)

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

    def store_file_to_volume(self, bucket, filepath):
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
        if os.path.exists(filepath):
            k.key = filepath
            k.set_contents_from_filename(filepath,
                                         cb=utils.percent_cb,
                                         num_cb=10)
            self.log.info("Uploaded file %s to bucket %s" % 
                         (filepath, bucket.name))

    def delete_volumes(self, buckets):
        '''
        Given a list of volumes to be deleted, remove all of them

        Attributes:
        ----------
        buckets: a list of S3 buckets
        '''
        for bucket in self.buckets:
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

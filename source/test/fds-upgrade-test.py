#!/usr/bin/python
from boto.exception import S3CreateError, S3ResponseError
import os
import sys
import argparse
import boto
from urlparse import urlparse
import hashlib
import requests
from boto.s3.connection import OrdinaryCallingFormat

class UpgradeTest:

    def __init__(self, opts):
        self.fdsuser = opts['fdsuser']
        self.fdspass = opts['fdspass']
        self.url = opts['url']
        self.authport = 7777
        self.s3port = 8000
        self.boto_debug = 0
        self.debug = opts['debug']
        if self.debug is True:
            self.boto_debug = 2
        self.user_token = self.get_user_token()
        self.bucket = 'upgradetest123'
        self.filename = 'upgradetestObject123'
        if 'file' in opts.keys():
            self.datafile = opts['file']
        self.s3conn = None
        self.get_s3conn()
        assert(self.s3conn is not None)

    def d(self, msg):
        if self.debug is True:
            print("D: %s" % msg)

    def get_user_token(self):
        url = '%s:%d/api/auth/token?login=%s&password=%s' % (self.url, self.authport, self.fdsuser, self.fdspass)
        self.d("Getting credentials from: %s" % url)
        r = requests.get(url)
        rjson = r.json()
        self.d("Got token: %s" % rjson['token'])
        return rjson['token']

    def get_s3conn(self):
        assert(self.s3conn is None)
        urlbits = urlparse(self.url)
        self.s3conn = boto.connect_s3(aws_access_key_id=self.fdsuser,
                                          aws_secret_access_key=self.user_token,
                                          host=urlbits.hostname,
                                          port=self.s3port,
                                          is_secure=False,
                                          calling_format=boto.s3.connection.OrdinaryCallingFormat(),
                                          debug=self.boto_debug)

    def delete_bucket(self):
        self.s3conn.delete_bucket(self.bucket)

    def create_bucket(self):
        self.s3conn.create_bucket(self.bucket)
        bucket_verify = self.s3conn.get_bucket(self.bucket)
        return bucket_verify

    def create_object(self, bucket):
        self.d("Creating object in bucket with name %s" % self.filename)
        newobject = bucket.new_key(self.filename)
        with open(self.datafile) as file:
            newobject.set_contents_from_file(file)

    def read_object(self, bucket):
        readobject = bucket.get_key(self.filename)
        assert(readobject is not None)
        return readobject.get_contents_as_string()

    def pre_upgrade_setup(self):
        bucket = self.create_bucket()
        self.create_object(bucket)
        obj = self.read_object(bucket)
        hash = hashlib.sha1(obj).hexdigest()
        print("Object successfully stored - hash: %s" % hash)

    def post_upgrade_verify(self):
        bucket = self.s3conn.get_bucket(self.bucket)
        self.d("Bucket: %s" % bucket)
        obj = self.read_object(bucket)
        self.d("Object: %s" % obj)
        hash = hashlib.sha1(obj).hexdigest()
        print("Object successfully read - hash: %s" % hash)

def setup(opts):
    ug_test = UpgradeTest(opts)
    ug_test.pre_upgrade_setup()

def verify(opts):
    ug_test = UpgradeTest(opts)
    ug_test.post_upgrade_verify()

def main():

    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest='sp_name', help='sub-command help')

    parser_setup = subparsers.add_parser('setup', help='pre-upgrade setup')
    parser_setup.add_argument('--file', dest="file", required=True,
                            help='Filename to store in FDS for testing')
    parser_setup.set_defaults(func=setup)

    parser_verify = subparsers.add_parser('verify', help='pre-upgrade verify')
    parser_verify.set_defaults(func=verify)

    parser.add_argument('--url', default='http://locahost',
                        help='url to run tests against')
    parser.add_argument('--fdsuser', dest="fdsuser", default='admin',
                        help='define FDS user to use Tenant API for auth token')
    parser.add_argument('--fdspass', dest="fdspass", default='admin',
                        help='define FDS pass to use Tenant API for auth token')
    parser.add_argument('--debug', action="store_true", default=False,
                        help='enable debugging')

    args         = parser.parse_args()
    opts = vars(args)
    args.func(opts)

if __name__ == '__main__':
    main()

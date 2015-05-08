#!/usr/bin/python

import requests
import boto
import os
import argparse
import logging
from subprocess import call
from urlparse import urlparse
from boto.s3.connection import OrdinaryCallingFormat
from boto.s3.key import Key

DEFAULT_URL = 'http://localhost:7777'
DEFAULT_USERNAME = 'admin'
DEFAULT_PASSWORD = 'admin'
DEFAULT_VERIFY = True
DEFAULT_S3PORT = 8000

class FDSVolumeMonitor:

    def __init__(self, username=DEFAULT_USERNAME, password=DEFAULT_PASSWORD,
                 url=DEFAULT_URL, verify=DEFAULT_VERIFY, s3port=DEFAULT_S3PORT,
                 debug=False):
        self.username = username
        self.password = password
        self.url = url
        self.debug = debug
        self.s3port = s3port
        p_url = urlparse(self.url)
        self.host = p_url.hostname
        if p_url.scheme == "https":
            self.secure = True
        else:
            self.secure = False
        self.verify = verify
        self.token = self.get_token()
        self.r_headers = {'FDS-Auth': self.token}
        self.logformat = "%(asctime)s:%(levelname)s:%(message)s"
        if self.debug == True:
            logging.basicConfig(format=self.logformat,level=logging.DEBUG)
            self.boto_debug = 2
        else:
            logging.basicConfig(format=self.logformat,level=logging.INFO)
            self.boto_debug = 0

    def get_token(self):
        """Obtain the auth token from the FDS Api and store in headers which
        are re-used on each request"""
        self.r_headers = {'FDS-Auth': ''}
        req = '%s/api/auth/token?login=%s&password=%s' % (
                self.url,
                self.username,
                self.password)
        logging.debug("Getting credentials from: {}".format(req))
        r = self.try_req('get', req)
        rjson = r.json()
        return rjson['token']

    def try_req(self, req_type, req):
        """Wrap calls to requests with a try to catch any exceptions thrown
        and expose HTTP errors - This method accepts any request type that
        the requests module support passed in via 'req_type' """
        try:
            call = getattr(requests, req_type)
            r = call(req, headers=self.r_headers, verify=self.verify)
        except requests.exceptions.RequestException as e:
            logging.error(e)
            sys.exit(1)
        return r

    def putGetOneObject(self, blobName, blockCount, bucket):
        devNull = open(os.devnull, 'w')
        call(["rm", "-f", ("/tmp/%s" % blobName)], stdin=devNull, stdout=devNull, stderr=devNull)
        call(["dd", "if=/dev/urandom", ("of=/tmp/%s" % blobName), "bs=4096", ("count=%s" % blockCount)],  stdin=devNull, stdout=devNull, stderr=devNull)
        key = bucket.new_key(blobName)
        key.set_metadata("some_metadata", "some_value")
        key.set_contents_from_filename("/tmp/%s" % blobName)
        call(["rm", "-f", ("/tmp/%s" % blobName)])
        key = bucket.get_key(blobName)
        self.assertTrue("Metadata not saved correctly", key.metadata['some_metadata'] == 'some_value')
        self.assertTrue("Wrong size for object", key.size == (blockCount * 4096))


    def execute(self):
        connection = boto.connect_s3(aws_access_key_id=self.username,
                                     aws_secret_access_key=self.token,
                                     host=self.host,
                                     port=self.s3port,
                                     is_secure=self.secure,
                                     debug=self.boto_debug,
                                     calling_format=boto.s3.connection.OrdinaryCallingFormat())
        bucket_name = 'canary'

        for bucket in connection.get_all_buckets():
            if bucket.name == bucket_name:
                for key in bucket.list():
                    bucket.delete_key(key)
                connection.delete_bucket(bucket_name)

        bucket = connection.create_bucket(bucket_name)

        self.putGetOneObject("smallBlob", 1, bucket)
        self.putGetOneObject("largeBlob", 1000, bucket)


    def assertTrue(self,message, condition):
        if not condition:
            logging.error(message)
            exit(-1)

def main():
    parser = argparse.ArgumentParser(description='Test creation of volumes & objects')
    parser.add_argument('--url', dest='url',
            default=DEFAULT_URL,
            help='URL for auth API (default: http://localhost:7777)')
    parser.add_argument('--user', dest='username',
            default=DEFAULT_USERNAME,
            help='API username (default: admin)')
    parser.add_argument('--pass', dest='password',
            default=DEFAULT_PASSWORD,
            help='API password (default: admin)')
    parser.add_argument('--s3port', dest='s3port',
            default=DEFAULT_S3PORT,
            help='S3 port (default: 8000)')
    parser.add_argument('--debug', dest='debug',
            action='store_true', default=False,
            help='Enable debug')
    parser.add_argument('-k', dest='verify',
            default=DEFAULT_VERIFY, action='store_false',
            help='Disable cert checking for SSL')
    args = parser.parse_args()
    monitor = FDSVolumeMonitor(url=args.url,
                               username=args.username,
                               password=args.password,
                               s3port=int(args.s3port),
                               verify=args.verify,
                               debug=args.debug
                              )
    monitor.execute()

if __name__ == '__main__':
    main()

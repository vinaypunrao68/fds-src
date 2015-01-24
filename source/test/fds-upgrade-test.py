#!/usr/bin/python
from boto.exception import S3CreateError, S3ResponseError
import os
import sys
import math
import argparse
import random
import boto
import time
import hashlib
import requests
from tempfile import NamedTemporaryFile
from boto.s3.connection import OrdinaryCallingFormat
from boto.s3.key import Key
from boto.s3.bucket import Bucket
from filechunkio import FileChunkIO
import pdb

def get_user_token(user, password, url):
    url = '%s/api/auth/token?login=%s&password=%s' % (url, user, password)
    print("Getting credentials from: ", url)
    r = requests.get(url)
    rjson = r.json()
    print(rjson)
    return rjson['token']


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument('--url', default='http://locahost:8000',
                        help='url to run tests against')
    parser.add_argument('--fds_user', dest="fds_user", default='admin',
                        help='define FDS user to use Tenant API for auth token')
    parser.add_argument('--fds_pass', dest="fds_pass", default='admin',
                        help='define FDS pass to use Tenant API for auth token')

    args         = parser.parse_args()
    # If we pass in --fds_pass then we use the API to get our token
    args.user_token = get_user_token(user=args.fds_user,
                                     password=args.fds_pass,
                                     url=args.url)

    print args.user_token

if __name__ == '__main__':
    main()

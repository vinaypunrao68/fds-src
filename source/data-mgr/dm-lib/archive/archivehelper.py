#!/usr/bin/python
# This script is for helping with archiving snapshot
import sys
import argh
import os
import subprocess
import logging
import boto
from boto.s3 import connection

logger = logging.getLogger('archivehelper')
hdlr = logging.FileHandler('archivehelper.log')
formatter = logging.Formatter('%(asctime)s %(levelname)s %(message)s')
hdlr.setFormatter(formatter)
logger.addHandler(hdlr) 
logger.setLevel(logging.INFO)

def createbucket(id, token, host, port, bucket_name):
    try:
        conn = boto.connect_s3(aws_access_key_id=id,
                               aws_secret_access_key=token,
                               host=host, port=int(port),
                               calling_format=boto.s3.connection.OrdinaryCallingFormat())
        bucket = conn.create_bucket(bucket_name)
        logger.info('bucket {} created'.format(bucket_name))
    except Exception, e:
        logger.error(e)
        sys.exit(1)
    sys.exit(0)

def put(id, token, host, port, bucket_name, object_name, file_path):
    try:
        conn = boto.connect_s3(aws_access_key_id=id,
                               aws_secret_access_key=token,
                               host=host, port=int(port),
                               calling_format=boto.s3.connection.OrdinaryCallingFormat())
        bucket = conn.get_bucket(bucket_name)
        k = bucket.new_key(object_name)
        num = k.set_contents_from_filename(file_path)
        logger.info('uploaded {} for file {}'.format(num, file_path))
    except Exception, e:
        logger.error(e)
        sys.exit(1)
    sys.exit(0)

def get(id, token, host, port, bucket_name, object_name, file_path):
    try:
        conn = boto.connect_s3(aws_access_key_id=id,
                               aws_secret_access_key=token,
                               host=host, port=int(port),
                               calling_format=boto.s3.connection.OrdinaryCallingFormat())
        bucket = conn.get_bucket(bucket_name)
        k = bucket.new_key(object_name)
        k.get_contents_to_filename(file_path)
        logger.info('downloaded file {}'.format(file_path))
    except Exception, e:
        logger.error(e)
        sys.exit(1)
    sys.exit(0)

parser = argh.ArghParser()
parser.add_commands([get, put])

if __name__ == '__main__':
    parser.dispatch()

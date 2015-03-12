#!/usr/bin/python
# This script is for helping with archiving snapshot
import sys
import argh
import os
import subprocess
import boto
from boto.s3 import connection

def get_access_key(host='127.0.0.1', user='admin', password='admin'):
    import restendpoint
    rest = restendpoint.RestEndpoint(host=host, port=7443,
                                     user=user, password=password, auth=False)
    token = rest.login(user, password)
    return token

def put(id, token, host, port, bucket_name, object_name, file_path):
    try:
        conn = boto.connect_s3(aws_access_key_id=id,
                               aws_secret_access_key=token,
                               host=host, port=int(port),
                               calling_format=boto.s3.connection.OrdinaryCallingFormat())
        bucket = conn.get_bucket(bucket_name)
        k = bucket.new_key(object_name)
        num = k.set_contents_from_filename(file_path)
        print 'uploaded {} for file {}' % (num, file_path)
    except Exception, e:
        print e
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
        print 'downloaded file {}' % (file_path)
    except Exception, e:
        print e
        sys.exit(1)
    sys.exit(0)

parser = argh.ArghParser()
parser.add_commands([get, put])

if __name__ == '__main__':
    parser.dispatch()

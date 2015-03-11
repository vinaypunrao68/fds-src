#!/usr/bin/python

import requests
import boto
import os
from subprocess import call
from boto.s3.connection import OrdinaryCallingFormat
from boto.s3.key import Key


def get_user_token(user, password, host, port, secure):
    if secure:
        proto = 'https'
    else:
        proto = 'http'
    url = '%s://%s:%d/api/auth/token?login=%s&password=%s' % (proto, host, port, user, password)
    r = requests.get(url, verify=False)
    rjson = r.json()
    return rjson['token']


def putGetOneObject(blobName, blockCount, bucket):
    devNull = open(os.devnull, 'w')
    call(["rm", "-f", ("/tmp/%s" % blobName)], stdin=devNull, stdout=devNull, stderr=devNull)
    call(["dd", "if=/dev/urandom", ("of=/tmp/%s" % blobName), "bs=4096", ("count=%s" % blockCount)],  stdin=devNull, stdout=devNull, stderr=devNull)
    key = bucket.new_key(blobName)
    key.set_metadata("some_metadata", "some_value")
    key.set_contents_from_filename("/tmp/%s" % blobName)
    call(["rm", "-f", ("/tmp/%s" % blobName)])
    key = bucket.get_key(blobName)
    assertTrue("Metadata not saved correctly", key.metadata['some_metadata'] == 'some_value')
    assertTrue("Wrong size for object", key.size == (blockCount * 4096))


def execute(user_token, host, port, secure):
    connection = boto.connect_s3(aws_access_key_id="admin",
                                 aws_secret_access_key=user_token,
                                 host=host,
                                 port=port,
                                 is_secure=secure,
                                 calling_format=boto.s3.connection.OrdinaryCallingFormat())
    bucket_name = 'canary'

    for bucket in connection.get_all_buckets():
        if bucket.name == bucket_name:
            for key in bucket.list():
                bucket.delete_key(key)
            connection.delete_bucket(bucket_name)

    bucket = connection.create_bucket(bucket_name)

    putGetOneObject("smallBlob", 1, bucket)
    putGetOneObject("largeBlob", 1000, bucket)


def assertTrue(message, condition):
    if not condition:
        print message
        exit(-1)

def main():
    user = "admin"
    password = "admin"
    host = "localhost"
    omPort = 7777
    s3Port = 8000
    secure = False
    user_token = get_user_token(user, password, host, omPort, secure)
    execute(user_token, host, s3Port, secure)

if __name__ == '__main__':
    main()

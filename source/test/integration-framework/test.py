#!/usr/bin/python
# coding: utf8
import boto
from boto.s3.key import Key
import boto.s3.connection

FDS_DEFAULT_KEY_ID            = 'AKIAJCNNNWKKBQU667CQ'
FDS_DEFAULT_SECRET_ACCESS_KEY = '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'

conn = boto.s3.connect_to_region('us-east-1',
       aws_access_key_id=FDS_DEFAULT_KEY_ID,
       aws_secret_access_key=FDS_DEFAULT_SECRET_ACCESS_KEY,
       is_secure=True,               # uncommmnt if you are not using ssl
       calling_format = boto.s3.connection.OrdinaryCallingFormat(),
)

for i in xrange(1, 31):
    name = "hello_" + str(i)
    conn.delete_bucket(name)

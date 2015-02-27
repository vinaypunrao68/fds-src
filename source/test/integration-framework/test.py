#!/usr/bin/python
# coding: utf8
from boto.s3.connection import S3Connection
import config
import config_parser
import utils
import s3
import sys

def percent_cb(complete, total):
    sys.stdout.write('=')
    sys.stdout.flush()

def test_download_file():
    fname = utils.download_file(config.REPOSITORY_URL)
    utils.untar_file(fname)

def test_hostname_to_ip():
    print utils.hostname_to_ip('fre-lxcnode-01')

def test_get_ips_from_inventory():
    print config_parser.get_ips_from_inventory('integration-framework-cluster')
    print config_parser.get_om_ipaddress_from_inventory('integration-framework-cluster')

def test_s3():
    s3conn = S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            None,
            '10.2.10.200',
            config.FDS_S3_PORT,
            '10.2.10.200'
        )
    s3conn.s3_connect()
    bucket = s3conn.conn.create_bucket('phil-bucket05-test')
    print bucket
    print "Sleeping 30 sec"
    time.sleep(30)
    testfile = "sample_file"
    print 'Uploading %s to Amazon S3 bucket %s' % \
       (testfile, bucket.name)
    k = Key(bucket)
    k.key = 'testfile'
    k.set_contents_from_filename(testfile,
            cb=percent_cb, num_cb=10)
    
    print "Downloading the file..."
    time.sleep(30)
    key = bucket.get_key('testfile')
    fp = open('testfile', 'w')
    key.get_file(fp)

if __name__ == "__main__":
    test_get_ips_from_inventory()
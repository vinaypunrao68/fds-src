#!/usr/bin/python
# coding: utf8
from filechunkio import FileChunkIO
from boto.s3.key import Key
import config
import config_parser
import lib
import select
import s3
import ssh
import sys
import time

import file_generator

def percent_cb(complete, total):
    sys.stdout.write('=')
    sys.stdout.flush()

def test_download_file():
    fname = lib.download_file(config.REPOSITORY_URL)
    lib.untar_file(fname)

def test_ssh_connection():
    local_ssh = ssh.SSHConn('10.2.10.200', config.SSH_USER,
                            config.SSH_PASSWORD)
    #cmd = "%s && %s" % (config.FDS_SBIN, config.START_ALL_CLUSTER)
    #local_ssh.channel.exec_command(cmd)
    #print local_ssh.channel.recv(1024)
    cluster = config.STOP_ALL_CLUSTER % config.FORMATION_CONFIG
    print cluster
    cmd = "cd %s && %s" % (config.FDS_SBIN, cluster)
    print cmd
    #cmd = "cd /fds/sbin && /fds-tool.py -f deploy_formation.conf -u"
    local_ssh.channel.exec_command(cmd)
    print local_ssh.channel.recv(1024)

def test_hostname_to_ip():
    print lib.hostname_to_ip('fre-lxcnode-01')

def test_get_ips_from_inventory():
    print config_parser.get_ips_from_inventory('integration-framework-cluster')
    print config_parser.get_om_ipaddress_from_inventory('integration-framework-cluster')

def test_hash_file_content():
    path = "./downloads/test_sample_1M"
    print lib.hash_file_content(path)

def test_concurrency():
    numbers = [1,2,3,4,5]
    lib.do_work(calculate_sqrt, numbers)
    
def calculate_sqrt(x):
    return x*x
    
def test_s3():
    s3conn = s3.S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            None,
            '10.1.16.111',
            config.FDS_S3_PORT,
            '10.1.16.111'
        )
    s3conn.s3_connect()
    bucket = s3conn.conn.create_bucket('phil-bucket05-test')
    print bucket
    print "Sleeping 30 sec"
    time.sleep(30)
    testfile = "test_2M"
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

def test_file_generator():
    fgen = file_generator.FileGenerator('4', 100, 'K')
    fgen.generate_files()

def test_mount_point():
    device ="/fds_100gb_block_volume"
    print lib.is_device_mounted(device)
    print lib.is_device_mounted("/filesystem")

def test_ssh_file():
    path = os.path.join(config.ANSIBLE_ROOT, "")
    sshconn = ssh.SSHConn(hostname, "ubuntu", None, "ssh.config")

def test_random_files():
    fgen = file_generator.FileGenerator('4', 100, 'K')
    print fgen.get_files()

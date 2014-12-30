#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import logging
import os
import random
import subprocess
import sys
import time
from subprocess import list2cmdline

import testsets.testcases.fdslib.TestUtils as TestUtils 

random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

def percent_cb(complete, total):
    sys.stdout.write('.')
    sys.stdout.flush()

def get_new_name_helper(key_name_prefix):
    return key_name_prefix + "_" + str(random.randint(0, 100000))

def create_buckets_helper(conn, param, bucket_name=None, count=1):

    assert(count > 0)
    buckets_list = []
    for i in range(count):
        if bucket_name is None:
            bucket_name = get_new_name_helper(param.bucket_prefix)
        try:
            log.info("creating bucket %s" % bucket_name)
            if count == 1:
                name = bucket_name
            else:
                name = bucket_name + "_" + str(i)
            conn.create_bucket(name)
            buckets_list.append(name)
        except Exception as e:
            log.info("Random bucket name should not exist %s" % bucket_name)
            raise
    return buckets_list


def create_keys_helper(conn, bucket_name, key_name_prefix, count,
                       src_file_name, metadata=None):
    # TODO: Take in a list of src_file_name, iterate through that list as 
    # source data.
    assert(count > 0)
    key_names = []
    res_keys  = []
    for i in range(count):
        key_names.append(get_new_name_helper(key_name_prefix))

    bucket = conn.get_bucket(bucket_name)
    with open(src_file_name) as fp:
        for key_name in key_names:
            key = bucket.new_key(key_name)
            if metadata:
                key.update_metadata(metadata)
            key.set_contents_from_file(fp)
            res_keys.append(key)
            fp.seek(0)
    return res_keys

def delete_all_keys_helper(conn, bucket_name):
        bucket  = conn.get_bucket(bucket_name)
        res_set = bucket.get_all_keys()
        res_set = bucket.delete_keys(res_set)

def get_user_token(user, password, host, port, secure, validate):
    if secure:
        proto = 'https'
    else:
        proto = 'http'

    url = '%s://%s:%d/api/auth/token?login=%s&password=%s' % (proto, host,port,
                                                              user, password)
    log.info("Getting credentials from: ", url)
    r = requests.get(url, verify=validate)
    rjson = r.json()
    log.info(rjson)
    return rjson['token']


def cpu_count():
    '''
    Returns the number of CPUs in the system

    Attributes:
    -----------
    None

    Returns:
    --------
    num_cpus : int
        the number of cpus in this machine
    '''
    num_cpus = 1
    if sys.platform == 'win32':
        try:
            num_cpus = int(os.environ['NUMBER_OF_PROCESSORS'])
        except (ValueError, KeyError):
            pass
    elif sys.platform == 'darwin':
        try:
            num_cpus = int(os.popen('sysctl -n hw.ncpu').read())
        except ValueError:
            pass
    else:
        try:
            num_cpus = os.sysconf('SC_NPROCESSORS_ONLN')
        except (ValueError, OSError, AttributeError):
            pass
    return num_cpus


def exec_commands(cmds):
    '''
    exec commands in parallel in multiple process
    (as much as we have CPU)

    Attributes:
    -----------
    cmds : list
        a list of commands to be executed

    Returns:
    --------
    None
    '''
    if not cmds:
        return  # empty list

    def done(p):
        return p.poll() is not None

    def success(p):
        return p.returncode == 0

    def fail():
        sys.exit(1)

    max_tasks = cpu_count()
    processes = []
    while True:
        while cmds and len(processes) < max_tasks:
            task = cmds.pop()
            print list2cmdline(task)
            processes.append(Popen(task))

        for p in processes:
            if done(p):
                if success(p):
                    processes.remove(p)
                else:
                    fail()

        if not processes and not cmds:
            break
        else:
            time.sleep(0.05)

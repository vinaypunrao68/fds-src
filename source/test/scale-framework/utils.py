#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import concurrent.futures
import fcntl
import logging
import hashlib
import os
import random
import re
import requests
import socket
import shutil
import struct
import subprocess
import sys
import tarfile
import time
import urllib2
from subprocess import list2cmdline

import config
import config_parser
import samples


random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

interfaces = ["eth0","eth1","eth2","wlan0","wlan1","wifi0","ath0","ath1","ppp0"]

def is_device_mounted(device):
    try:
        device = subprocess.check_output("grep '%s' /proc/mounts | awk '{printf $1}'" % device, shell=True)
        if len(device) == 0:
            return False
        return True
    except Exception, e:
        log.exception(e)
        sys.exit(2)

def create_s3_connection(om_ip, am_ip, auth=None):
    '''
    Given the two ip addresses (OM and AM nodes), establish a S3 connection to
    the FDS cluster

    Attributes:
    -----------
    om_ip: The OM Node IP address
    am_ip: the AM node IP address
    '''
    s3conn = s3.S3Connection(
            config.FDS_DEFAULT_ADMIN_USER,
            auth,
            om_ip,
            config.FDS_S3_PORT,
            am_ip
    )
    s3conn.s3_connect()
    return s3conn

def execute_cmd(cmd):
    try:
        return subprocess.check_output([cmd], shell=True)
    except Exception, e:
        log.exception(e)
        sys.exit(2)

def do_work(function, params):
    with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
        results = {executor.submit(function, param): param for param in params}
        for result in concurrent.futures.as_completed(results):
            current = results[result]
            try:
                data = result.result()
            except Exception as exc:
                log.info('%r generated an exception: %s' % (current, exc))
            else:
                log.info('%r ---- %s' % (current, data))

def hostname_to_ip(hostname):
    return socket.gethostbyname(hostname)

def create_test_files_dir():
    create_dir(config.TEST_DIR)
    # data files aren't there, download them
    if os.listdir(config.TEST_DIR) == []:
        fname = download_file(config.REPOSITORY_URL)
        untar_file(fname)

def untar_file(fname):
    if (fname.endswith("tar.gz")):
        tar = tarfile.open(fname)
        tar.extractall()
        tar.close()
        log.info("Extracted in %s" % fname)
    else:
        log.warning("Not a tar.gz file: '%s'" % fname)


def download_file(url):
    f = urllib2.urlopen(url)
    name = get_file_name_from_url(url)
    create_dir(config.DOWNLOAD_DIR)
    source = os.path.join(config.DOWNLOAD_DIR, name)
    log.info("Downloading to %s" % source)
    with open(source, "wb") as code:
        code.write(f.read())
    return source

def create_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

def remove_dir(path):
    if os.path.exists(path):
        shutil.rmtree(path)

def get_file_name_from_url(url):
    return url.strip().split('/')[-1]

def hash_file_content(path):
    '''
    Hash the file content using MD5, and store the hash key for later. If
    The file content changed, or was corrupted, the hash key will show it.

    Attributes:
    -----------
    fname : str
        the name of the file to be hashed

    Returns:
    --------
    str: a MD5 hash key
    '''
    try:
        if not os.path.exists(path):
            raise IOError, "File does not exist"
        m = hashlib.md5()
        with open(path, 'r') as f:
            data = f.read(1024)
            if len(data) == 0:
                return None
            m.update(data)
            return m.hexdigest()
        return encode
    except Exception, e:
        log.exception(e)
            
def create_file_sample(fname, size, unit="M"):
    '''
    Create a sample data file for testing purpose.
    
    Attributes:
    -----------
    fname : str
        the same of the file to be created
    size : int
        the size of the file, in unit
    unit : str
        specify the file's size, either in Megabytes(M) or Gigabytes (G)
    
    Returns:
    --------
    bool : True or False, indicating if the file was created successful.
    '''
    try:
        if unit not in ["M", "G"]:
            log.warning("Invalid Unit size. Must be Megabytes(M) or " \
                "Gigabytes(G). Will default to Megabytes")
            unit = "M"

        if unit == "M":
            return samples.sample_mb_files
        else:
            return samples.sample_gb_files
        path = os.path.join(config.SAMPLE_DIR, fname)
        cmd = "fallocate -l %s%s %s" % (size, unit, path)
        subprocess.call([cmd], shell=True)
        log.info("Created file %s of size %s%s" % (fname, size, unit))
        return True
    except Exception, e:
        log.exception(e)
        return False
        
def is_valid_ip(ip):
    '''
    Check if an user specified ip address is valid.
    
    Attributes:
    -----------
    ip : str
        the ip address to be sanitized
    
    Returns:
    --------
    bool : if the ip address is valid or not
    '''
    m = re.match(r"^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$", ip)
    return bool(m) and all(map(lambda n: 0 <= int(n) <= 255, m.groups()))

def get_ip_address(ifname):
    '''
    Given the eth0 or eth1 of the local host machine
    picks up its ip address
    
    Arguments:
    ----------
    ifname : str
        ethernet card, such as eth0
    
    Returns:
    --------
    str : the local machine's ip address.
    '''
    if ifname not in interfaces:
        raise ValueError, "Unknown interface %s" % ifname
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa(fcntl.ioctl(
            s.fileno(),
            0x8915, # SIOCGIFADDR
            struct.pack('256s', ifname[:15])
        )[20:24])
         
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
    #log.info("Getting credentials from: ", url)
    r = requests.get(url, verify=validate)
    rjson = r.json()
    #log.info(rjson)
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

def compare_hashes(dict1_hashes, dict2_hashes):
    '''
	This function compares two dictionary hashes.

    Attributes:
    -----------
	Takes two dictionary hashes

    Returns:
    --------
	Returns true if two dictionary hashes are the same
    '''

    for k, v in dict1_hashes.iteritems():

	    log.info("Comparing hashes for file %s" % k)
	    if dict1_hashes[k]  != dict2_hashes[k]:
		log.warning("%s != %s - on key=%s" % (dict1_hashes[k], dict2_hashes[k], k))
		return False
		break

    return True

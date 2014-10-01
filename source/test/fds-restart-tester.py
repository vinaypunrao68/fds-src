#!/usr/bin/python
# coding=UTF8
'''
Created on Sep 3, 2014

@author: dave

fds-restart-tester.py
1. Simple I/O and restart
  Graceful shutdown/Bring up of Node
  ▪ Steps: start I/O. Stop all I/O [optional].
  ▪ Steps: From CLI/command line, stop FDS domain. Next, restart the
    domain.
  ▪ Steps: Read back all blobs that were committed during the initial write.
  ▪ Expected Result: For all

2. On-going GC and restart
  Graceful shutdown/Bring up of Node
  ▪ Steps: start I/O. Start GC.
  ▪ Steps: From CLI/command line, stop FDS domain. Next, restart the
    domain.
  ▪ Steps: Read back all blobs that were committed during the initial write.
  ▪ Expected Result: For all blobs that were committed, they should still exist
    after domain restart.

		1. use fds-tool.py -i -f <cfg file> to install our software to (3 to 4 node) domain
		2. fds-tool.py -u -v -f <cfg file> to bring up the the domain
		3.  Create bunch of volumes and issue bunch of put requests
		4. fds-tool.py -d -v -f <cfg file> to bring the domain down (note no cleanup is done)
[Sep-3 2:31 PM] Dave Setzke: is there a tool that generates volumes/data?
[Sep-3 2:31 PM] Rao Bayyana: 5. fds-tool.py --restart -v -f <cfg file> to restart domain
		check with Nick eidler on that
[Sep-3 2:31 PM] Dave Setzke: OK
[Sep-3 2:32 PM] Rao Bayyana: 6. Do bunch of get requests for the blobs that you'd put before and make sure they succeed
		For number 6, ideally we should run checker, which checks the data is consistent between all the nodes.  But I am not sure if that tool is functional.
		For you now you can have stub function to run checker
'''

import os, sys
import random
import time
import shutil
import hashlib
import fdslib.FdsSetup as inst
import fdslib.BringUpCfg as fdscfg
import fdslib.BasicCluster as BasicCluster
import threading
import requests

import boto
from boto.s3.connection import OrdinaryCallingFormat
from boto.s3.key import Key
from boto.exception import S3CreateError

import logging
from posix import mkdir
logging.basicConfig(filename="/tmp/boto.log", level=logging.DEBUG)

import optparse
import unittest

DEBUG = True
def debug(text, output=sys.stdout):
    if DEBUG:
        print >> output, text

def log(text, output=sys.stdout):
    print >> output, text

# separator chars used to delimit a directory path / char
# in s3 keys.  The / char is interpreted as a bucket path
KEY_DIR_SEP='_:_'

class RestartTest(object):
    '''
    Test restart functionality
    '''

    def __init__(self, cfg, nodes, ams, cli):
        '''
        Constructor
        '''
        print "Restart Tester - init"
        self.cfg = cfg
        self.nodes = nodes
        self.ams = ams
        self.cli = cli
        self.conn = None
        self.commitlog_base = '/tmp/restart-test-commit'
        self.buckets_clogs = {}
        self.cluster = BasicCluster.basic_cluster (nodes, ams, cli)

    def __fileUploaded(self, bucket, srcpath, keyname, sha1):
        '''
        Writes a line to a bucket commit log containing the keyname, source path, and source sha1 hash
        '''
        clog = self.buckets_clogs.get(bucket)
        if clog is None:
            clog = open(self.commitlog_base + '_' + bucket + ".log", "a")
            self.buckets_clogs[bucket] = clog
        clog.write('%s|%s|%s\n' % (keyname, srcpath, sha1))
        clog.flush()

    def nodeStatusAll(self):
        self.cluster.status()

    ## TODO: this was pulled from fds-tool.py.  It should be a common lib function
    def startNodes(self):
        om = self.cfg.rt_om_node
        om_ip = om.nd_conf_dict['ip']

        for n in self.nodes:
            n.nd_agent.ssh_exec('python -m disk_type -m', wait_compl=True)
            n.nd_agent.ssh_exec('/fds/sbin/redis.sh start', wait_compl=True, output=True)

        # TODO: not sure if it is necessary that redis is started
        # on all nodes before platform is started on any of them
        for n in self.nodes:
            n.nd_start_platform(om_ip)

        print "Start OM on IP", om_ip
        om.nd_start_om()
        time.sleep(8)

        # activate nodes activates all services in the domain. Only
        # need to call it once
        """
        for n in self.nodes:
            if n.nd_conf_dict['node-name'] == 'node1':
                self.cli.run_cli('--activate-nodes abc -k 1 -e sm,dm')
            else:
                self.cli.run_cli('--activate-nodes abc -k 1 -e sm,dm')
            print "Waiting for node %s to come up" % n.nd_host
            time.sleep(3)
        """

        self.cli.run_cli('--activate-nodes abc -k 1 -e sm,dm')
        print "Waiting for services to come up"
        time.sleep(8)

        for am in self.ams:
            am.am_start_service()
        print "Waiting for all commands to complete before existing"
        time.sleep(30) # Do not remove this sleep

    ## TODO: this was pulled from fds-tool.py.  should be a common lib function
    def shutdownNodes(self, clean=0):
        ''' cleanup/shutdown all of the nodes to prepare for test '''
        for n in self.nodes:
            self.shutdownNode(n, clean)

    def shutdownNode(self, n, clean=0):
        # shutdown/cleanup fds daemon processes
        n.nd_cleanup_daemons()

        # cleanup nodes
        if clean != 0:
            n.nd_cleanup_node()
            n.nd_agent.ssh_exec('(cd /fds/sbin && ./redis.sh clean)', output=True)
            n.nd_agent.ssh_exec('rm /fds/uuid_port')

        # todo(Rao): Unregister the services here.  Once we implement name based
        # services this should be easy
        #print("NOTE: Run fdscli --remove-services to remove services")

    def __connect(self):
        ''' connect to the fds server via s3 API '''
        if not self.conn is None:
            return self.conn;
        om = self.cfg.rt_om_node
        om_ip = om.nd_conf_dict['ip']
        portnum = 8000 #om.nd_conf_dict['fds_port']
        debug("Connecting to %s" % om_ip + ':' + str(portnum))
        at = self.getAuthToken('admin', 'admin')
        ## This assumes authentication is NOT enabled on the cluster!
        self.conn = boto.connect_s3(
            aws_access_key_id='admin',
            #aws_secret_access_key='yASvtWBUa8o2R+cr1jZqWEoCSmQ8kK/ofeZdC3zW4fiCKCKuToP13KoaObVMIHynMeE7BU8qCdNwkPSthgQsVw==',
            aws_secret_access_key=at,
            host=om_ip,
            port=int(portnum),
            is_secure=False,
            calling_format=boto.s3.connection.OrdinaryCallingFormat())
        return self.conn

    def __close(self):
        if self.conn is None:
            return
        c = self.conn
        self.conn = None
        try:
            c.close();
        except Exception as ex:
            print(ex)

    def getAuthToken(self, user, pwd):
        om = self.cfg.rt_om_node
        om_ip = om.nd_conf_dict['ip']

        # TODO: need to get the admin_webapp_port from the platform.conf.
        portnum = 7777 #om.nd_conf_dict['admin_webapp_port']
        #http://10.1.33.101:7777/api/auth/token?login=admin&password=admin'
        url = 'http://%s:%d/api/auth/token?login=%s&password=%s' % (om_ip, portnum, user, pwd)
        debug(url)
        r = requests.get(url)
        rjson = r.json()
        debug(rjson)
        return rjson['token']

    def sendS3IO(self, bucketName):
        ''' Send a bunch of IO to the cluster '''
        c = self.__connect()
        try:
            b = c.create_bucket(bucketName)
            debug('Bucket <%s> created!' % bucketName)
            self.__upload_dir(b, '/projects/fds-src/source/test/fdslib')
        finally:
            self.__close()

    def __upload_file(self, b, src):
        f = open(src, "r")
        # TODO: skipping zero-length files - causes AM crash (WIN-929)
        if os.path.getsize(src) == 0:
            debug("- file %s is zero length - skipping" % (src))
            return
        debug("- uploading file %s to bucket %s" % (src,b))
        try:
            k = Key(b)
            k.key = str(os.path.relpath(src)).replace('/', KEY_DIR_SEP)
            k.set_contents_from_file(f)

            hash = hashlib.sha1(open(src, 'rb').read()).hexdigest();
            ## TODO: on success append path to a temp file that
            # we can use to validate.  This will be important for
            # the second test where we shutdown while IO is processing
            self.__fileUploaded(b.name, src, k.key, hash)
        # except Exception as inst:
        #     debug('Failed ' + str(inst))
        #     raise inst
        finally:
            f.close()

    def __upload_dir(self, b, path):
        debug("- uploading dir %s" % path)
        exclude = []
        for f in os.listdir(path):
            if f not in exclude:
                if os.path.isdir("%s/%s" % (path, f)) is True:
                    self.__upload_dir(b, "%s/%s" % (path, f))
                else:
                    self.__upload_file(b, "%s/%s" % (path, f))

    def validateUploadedFiles(self, bucketName):
        c = self.__connect()
        bucket = c.get_bucket(bucketName)
        fn = self.commitlog_base + "_" + bucket.name + ".log"
        debug('validating uploaded files from %s' % fn)
        clf = open(fn, "r")
        outdir = self.commitlog_base +"_" + bucket.name
        if os.path.exists(outdir):
            shutil.rmtree(outdir)
        os.makedirs(outdir)

        for line in clf:
            line = line.rstrip("\r\n")
            debug(line)
            tokens = line.split('|')
            debug(tokens)
            tkey = tokens[0]
            tsrc = tokens[1]
            tsha = tokens[2]

            outfile = os.path.join(outdir, tsrc.lstrip("/"))
            outfdir = os.path.dirname(outfile)
            if not os.path.exists(outfdir):
                os.makedirs(outfdir, 0770)
            debug("Retrieving %s(%s) to %s" % (bucket.name,tkey,outfile))
            k = Key(bucket)
            k.key = tkey
            k.get_contents_to_filename(outfile)

            valsha = hashlib.sha1(open(outfile, 'rb').read()).hexdigest()
            debug('validating %s hash %s = %s' % (tsrc, tsha, valsha))
            assert(tsha == valsha)

    def newBucketName(self):
        return 'bucket' + str(random.randint(0, 100000))

class IOThread (threading.Thread):
    def __init__(self, name, rtest, bucket_name):
        threading.Thread.__init__(self)
        self.name = name
        self.rtest = rtest
        self.bucketName = bucket_name

    def run(self):
        print "Starting " + self.name
        self.rtest.sendS3IO(self.bucketName)
        print "Exiting " + self.name


class TestRestartTest(unittest.TestCase):

    def setUp(self):
        # shutdown and clean cluster
        r.shutdownNodes(1)

        # start nodes
        r.startNodes()

    def test_io_persists_shutdown(self):
        # start IO
        bucketName = r.newBucketName()
        r.sendS3IO(bucketName)
        r.validateUploadedFiles(bucketName)

        r.shutdownNodes(0)
        r.startNodes()

        r.validateUploadedFiles(bucketName)

    def test_mt_io_persists_shutdown(self):
        # start IO
        bucketName = r.newBucketName()
        ioth = IOThread("RestartTestIO", r, bucketName)
        ioth.start()

        # let the io thread run for a little while before
        # we shutdown the server
        time.sleep(10)

        # shutdown the server
        r.shutdownNodes(0)

        # restart the server
        r.startNodes()

        # validate that the files uploaded successfully
        # before the server was shutdown can all be accessed
        # and have expected sha1 hash
        r.validateUploadedFiles(bucketName)


def main():
    global r
    print "Restart Tester - main"

    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option('-f', '--file', dest = 'config_file',
                      help = 'configuration file (e.g. sample.cfg)', metavar = 'FILE')
    parser.add_option('-v', '--verbose', action = 'store_true', dest = 'verbose',
                      help = 'enable verbosity')
    parser.add_option('-R', '--fds-root', dest = 'fds_root', default = '/fds',
                      help = 'specify fds-root directory')
    parser.add_option('-r', '--dryrun', action = 'store_true', dest = 'dryrun',
                      help = 'dry run, print commands only')
    parser.add_option('-t', '--tests', dest = 'run_tests', help = 'List of tests to run.  All if not provided')
    (options, args) = parser.parse_args()

    # Packaging
    env = inst.FdsEnv('.')
    if options.config_file is None:
        print "Config file (-f|--file) is required"
        sys.exit(0)

    cfg = fdscfg.FdsConfigRun(None, options)

    # Get all the configuration
    nodes = cfg.rt_get_obj('cfg_nodes')
    ams   = cfg.rt_get_obj('cfg_am')
    cli   = cfg.rt_get_obj('cfg_cli')
    steps = cfg.rt_get_obj('cfg_scenarios')

    r = RestartTest(cfg, nodes, ams, cli)

    suite = unittest.TestLoader().loadTestsFromTestCase(TestRestartTest)
    unittest.TextTestRunner(verbosity=2).run(suite)

if __name__ == '__main__':
    main()

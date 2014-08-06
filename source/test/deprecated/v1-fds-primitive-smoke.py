#!/usr/bin/env python

import os, sys
import inspect
import argparse
import subprocess
from multiprocessing import Process
import ServiceMgr
import ServiceConfig
import pdb
import time

class FdsEnv:
    def __init__(self, _root):
        self.env_cdir      = os.getcwd()
        self.srcdir    = ''
        self.env_root      = _root
        self.env_exitOnErr = True
        self.total_put     = 0
        self.total_get     = 0
        
        # assuming that this script is located in the test dir
        self.testdir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
        self.srcdir=os.path.abspath(self.testdir + "/..")
        self.toolsdir = os.path.abspath(self.srcdir + "/tools")
        self.fdsctrl  = self.toolsdir + "/fds"

        if self.srcdir == "":
            print "Can't determine FDS root from the current dir ", self.env_cdir
            sys.exit(1)

    def shut_down(self):
         subprocess.call([self.fdsctrl,'stop'])

    def cleanup(self):
        self.shut_down()
        subprocess.call([self.fdsctrl,'clean'])
        subprocess.call([self.fdsctrl,'clogs'])

    def getRoot(self):
        return self.env_root

# Setup test environment, directories, etc
class FdsSetupNode:
    def __init__(self, fdsSrc, fds_data_path):
        try:
            os.mkdir(fds_data_path)
        except:
            pass
        subprocess.call(['cp', '-rf', fdsSrc + '/config/etc', fds_data_path])

class FdsDataSet:
    def __init__(self, bucket='abc', data_dir='', dest_dir='/tmp', file_ext='*'):
        self.ds_bucket   = bucket
        self.ds_data_dir = data_dir
        self.ds_file_ext = file_ext
        self.ds_dest_dir = dest_dir

        if not self.ds_data_dir or self.ds_data_dir == '':
            self.ds_data_dir = os.getcwd()

        if not self.ds_dest_dir or self.ds_dest_dir == '':
            self.ds_dest_dir = '/tmp'
        ret = subprocess.call(['mkdir', '-p', self.ds_dest_dir])

#########################################################################################


class CopyS3Dir:
    global_obj_prefix = 0
    def __init__(self, dataset):
        self.ds         = dataset
        self.verbose    = False
        self.bucket     = dataset.ds_bucket
        self.exitOnErr  = True
        self.files_list = []
        self.cur_put    = 0
        self.cur_get    = 0
        self.perr_cnt   = 0
        self.gerr_cnt   = 0
        self.gerr_chk   = 0
        self.obj_prefix = str(CopyS3Dir.global_obj_prefix)
        CopyS3Dir.global_obj_prefix += 1
        self.bucket_url = 'http://localhost:8000/' + self.bucket

        os.chdir(self.ds.ds_data_dir)
        files = subprocess.Popen(
            ['find', '.', '-name', self.ds.ds_file_ext, '-type', 'f', '-print'],
            stdout=subprocess.PIPE
        ).stdout
        for rec in files:
            rec = rec.rstrip('\n')
            self.files_list.append(rec)
        if len(self.files_list) == 0:
            print "WARNING: Data directory is empty"

    def resetTest(self):
        self.cur_put    = 0
        self.cur_get    = 0

    def runTest(self, burst_cnt=0):
        self.createBucket()
        total = len(self.files_list)
        if burst_cnt == 0 or burst_cnt > total:
            burst_cnt = total
        cnt = 0
        while cnt < total:
            self.putTest(burst_cnt)
            self.getTest(burst_cnt)
            cnt += burst_cnt

    def createBucket(self):
        ret = subprocess.call(['curl', '-X' 'PUT', self.bucket_url])
        if ret != 0:
            print "Bucket create failed"
            sys.exit(1)
        subprocess.call(['sleep', '3'])

    def printDebug(self, message="INFO: "):
        if self.verbose:
            print message

    def putTest(self, burst=0):
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        if burst <= 0:
            burst = len(self.files_list)
        for put in self.files_list[self.cur_put:]:
            cnt = cnt + 1
            obj = self.obj_prefix + put.replace("/", "_")
            self.printDebug("curl PUT: " + put + " -> " + obj)
            ret = subprocess.call(['curl', '-X', 'PUT', '--data-binary',
                                   '@' + put, self.bucket_url + '/' + obj])
            if ret != 0:
                err = err + 1
                print "curl error PUT: " + put + ": " + obj
                if self.exitOnErr == True:
                    sys.exit(1)

            if (cnt % 10) == 0:
                print "Put ", cnt + self.cur_put , " objects... errors: [", err, "]"
            if cnt == burst:
                break

        print "Put ", cnt + self.cur_put, " objects... errors: [", err, "]"
        self.cur_put  += cnt
        self.perr_cnt += err
        #self.env.total_put += cnt

    def getTest(self, burst=0):
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        chk = 0
        if burst <= 0:
            burst = len(self.files_list)
        for get in self.files_list[self.cur_get:]:
            cnt = cnt + 1
            obj = self.obj_prefix + get.replace("/", "_");
            tmp = self.ds.ds_dest_dir + '/' + obj
            self.printDebug("curl GET: " + obj + " -> " + tmp)
            ret = subprocess.call(['curl', '-s' '1' ,
                                   self.bucket_url + '/' + obj, '-o', tmp])
            if ret != 0:
                err = err + 1
                print "curl error GET: " + get + ": " + obj + " -> " + tmp
                if self.exitOnErr == True:
                    sys.exit(1)

            ret = subprocess.call(['diff', get, tmp])
            if ret != 0:
                chk = chk + 1
                print "Get ", get, " -> ", tmp, ": ", cnt,  \
                      " objects... errors: [", err, "], verify failed [", chk, "]"
                if self.exitOnErr == True:
                    sys.exit(1)
            else:
                subprocess.call(['rm', tmp])
            if (cnt % 10) == 0:
                print "Get ", cnt + self.cur_get, \
                      " objects... errors: [", err, "], verify failed [", chk, "]"
            if cnt == burst:
                break

        print "Get ", cnt + self.cur_get, " objects... errors: [", err, \
              "], verify failed [", chk, "]"
        self.cur_get  += cnt
        self.gerr_cnt += err
        self.gerr_chk += chk
        #self.env.total_get += cnt

    def exitOnError(self):
        if self.perr_cnt != 0 or self.gerr_cnt != 0 or self.gerr_chk != 0:
            print "Test Failed: put errors: [", self.perr_cnt,  \
                  "], get errors: [", self.gerr_cnt,            \
                  "], verify failed [", self.err_chk, "]"
            sys.exit(1)

class CopyS3Dir_GET(CopyS3Dir):
    def __init__(self, dataset):
        CopyS3Dir.__init__(self, dataset)
# CopyS3Dir.global_obj_prefix -= 1
        self.obj_prefix = str(CopyS3Dir.global_obj_prefix)

    def runTest(self, write=1, read=1):
        total = len(self.files_list)
        if read == 0 or read > total:
            read = total

        rd_cnt = 0
        while rd_cnt < total:
            self.getTest(read)
            rd_cnt += read

    def runTestAll(self):
        self.runTest()

        self.resetTest()
        self.obj_prefix = str(2)
        self.runTest()

        self.resetTest()
        self.obj_prefix = str(3)
        self.runTest()

        self.resetTest()
        self.obj_prefix = str(4)
        self.runTest()

        self.resetTest()
        self.obj_prefix = str(5)
        self.runTest()

        self.resetTest()
        self.obj_prefix = str(6)
        self.runTest()


class CopyS3Dir_PatternRW(CopyS3Dir):
    def runTest(self, write=1, read=1):
        self.createBucket()
        total = len(self.files_list)
        if write == 0 or write > total:
            write = total

        if read == 0 or read > total:
            read = total

        wr_cnt = 0
        rd_cnt = 0
        while wr_cnt < total and rd_cnt < total:
            self.putTest(write)
            self.getTest(read)
            wr_cnt += write
            rd_cnt += read

        if wr_cnt < total:
            self.putTest()
        if rd_cnt < total:
            self.getTest()

class CopyS3Dir_Overwrite(CopyS3Dir):
    def runTest(self):
        CopyS3Dir.runTest(self)

        self.putTest()
        self.getTest()

    def putTest(self, burst=0):
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        if len(self.files_list) > 0:
            wr_file = self.files_list[0]
        for put in self.files_list:
            cnt = cnt + 1
            obj = self.obj_prefix + put.replace("/", "_")
            self.printDebug("curl PUT: " + put + " -> " + obj)
            ret = subprocess.call(['curl', '-X', 'PUT', '--data-binary',
                    '@' + wr_file, self.bucket_url + '/' + obj])
            if ret != 0:
                err = err + 1
                print "curl error PUT: " + put + ": " + obj
                if self.exitOnErr == True:
                    sys.exit(1)

            if (cnt % 10) == 0:
                print "Put ", cnt + self.cur_put , " objects... errors: [", err, "]"

        print "Put ", cnt + self.cur_put, " objects... errors: [", err, "]"
        self.cur_put  += cnt
        self.perr_cnt += err

    def getTest(self, burst=0):
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        chk = 0
        if len(self.files_list):
            rd_file = self.files_list[0]
        for get in self.files_list:
            cnt = cnt + 1
            obj = self.obj_prefix + get.replace("/", "_");
            tmp = self.ds.ds_dest_dir + '/' + obj
            self.printDebug("curl GET: " + obj + " -> " + tmp)
            ret = subprocess.call(['curl', '-s' '1' ,
                                   self.bucket_url + '/' + obj, '-o', tmp])
            if ret != 0:
                err = err + 1
                print "curl error GET: " + get + ": " + obj + " -> " + tmp
                if self.exitOnErr == True:
                    sys.exit(1)

            ret = subprocess.call(['diff', rd_file, tmp])
            if ret != 0:
                chk = chk + 1
                print "Get ", get, " -> ", tmp, ": ", cnt,  \
                      " objects... errors: [", err, "], verify failed [", chk, "]"
                if self.exitOnErr == True:
                    sys.exit(1)
            else:
                subprocess.call(['rm', tmp])
            if (cnt % 10) == 0:
                print "Get ", cnt + self.cur_get, \
                      " objects... errors: [", err, "], verify failed [", chk, "]"
            if cnt == burst:
                break

        print "Get ", cnt + self.cur_get, " objects... errors: [", err, \
              "], verify failed [", chk, "]"
        self.cur_get  += cnt
        self.gerr_cnt += err
        self.gerr_chk += chk
#self.env.total_get += cnt



#########################################################################################
## Cluster bring up
## ----------------
def bringupCluster(env, bu, cfgFile, verbose, debug):
    env.cleanup()
    print "\nSetting up private fds-root in " + bu.getCfgField('node1', 'fds_root') + '[1-4]'
    root1 = bu.getCfgField('node1', 'fds_root')
    root2 = bu.getCfgField('node2', 'fds_root')
    root3 = bu.getCfgField('node3', 'fds_root')
    root4 = bu.getCfgField('node4', 'fds_root')
    FdsSetupNode(env.srcdir, root1)
    FdsSetupNode(env.srcdir, root2)
    FdsSetupNode(env.srcdir, root3)
    FdsSetupNode(env.srcdir, root4)
    
    os.chdir(env.srcdir + '/Build/linux-x86_64.debug/bin')

    print "\n\nStarting fds on ", root1
    subprocess.call([env.fdsctrl,'start'])
    os.chdir(env.srcdir)

def startSM2(args):
    print "\n\nStarting SM on node2...."
    subprocess.Popen(['./StorMgr', '--fds-root', args.root + '/node2', '--fds.sm.data_port=7911',
                      '--fds.sm.control_port=6911', '--fds.sm.prefix=node2_', '--fds.sm.test_mode=false',
                      '--fds.sm.log_severity=0', '--fds.sm.om_ip=127.0.0.1', '--fds.sm.migration.port=8610', '--fds.sm.id=sm2'],
                     stderr=subprocess.STDOUT)
    subprocess.call(['sleep', '3'])

def startSM3(args):
    print "\n\nStarting SM on node3...."
    subprocess.Popen(['./StorMgr', '--fds-root', args.root + '/node3', '--fds.sm.data_port=7921',
                      '--fds.sm.control_port=6921', '--fds.sm.prefix=node3_', '--fds.sm.test_mode=false',
                      '--fds.sm.log_severity=0', '--fds.sm.om_ip=127.0.0.1', '--fds.sm.migration.port=8620', '--fds.sm.id=sm3'],
                     stderr=subprocess.STDOUT)
    subprocess.call(['sleep', '3'])

def startSM4(args):
    print "\n\nStarting SM on node4...."
    subprocess.Popen(['./StorMgr', '--fds-root', args.root + '/node4', '--fds.sm.data_port=7931',
                      '--fds.sm.control_port=6931', '--fds.sm.prefix=node4_', '--fds.sm.test_mode=false',
                      '--fds.sm.log_severity=0', '--fds.sm.om_ip=127.0.0.1', '--fds.sm.migration.port=8630', '--fds.sm.id=sm4'],
                     stderr=subprocess.STDOUT)
    subprocess.call(['sleep', '3'])

USER_NAME = 'bao_pham'
USER_SSH_KEY = '/tmp/lab.rsa'

def bringupClusterCLI(env, bu, cfgFile, verbose, debug):
    env.cleanup()
    print "\nSetting up private fds-root in " + bu.getCfgField('node1', 'fds_root') + ' [1-4]'
    root1 = bu.getCfgField('node1', 'fds_root')
    root2 = bu.getCfgField('node2', 'fds_root')
    root3 = bu.getCfgField('node3', 'fds_root')
    root4 = bu.getCfgField('node4', 'fds_root')
    FdsSetupNode(env.srcdir, root1)
    FdsSetupNode(env.srcdir, root2)
    FdsSetupNode(env.srcdir, root3)
    FdsSetupNode(env.srcdir, root4)

    print "\n\nStarting Cluster on node1...."
    p = subprocess.Popen(['test/bring_up.py', '--file', cfgFile, '--verbose', verbose, '--debug', debug, '--up', '--ssh_user', USER_NAME, '--ssh_passwd', 'passwd', '--ssh_key', USER_SSH_KEY],
#    p = subprocess.Popen(['test/bring_up.py', '--file', cfgFile, '--verbose', verbose, '--debug', debug, '--up'],
                         stderr=subprocess.STDOUT)
    p.wait()
    print "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

def bringupNode(env, bu, cfgFile, verbose, debug, node):
    root1 = bu.getCfgField('node1', 'fds_root')
    root2 = bu.getCfgField('node2', 'fds_root')
    root3 = bu.getCfgField('node3', 'fds_root')
    root4 = bu.getCfgField('node4', 'fds_root')

    print "\n\nBringup new node.... ", node
    p = subprocess.Popen(['test/bring_up.py', '--file', cfgFile, '--verbose', verbose, '--debug', debug, '--up', '--section', node],
                         stderr=subprocess.STDOUT)
    p.wait()
    print "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"



#########################################################################################
## I/O Load generation
## -------------------
def preCommit(volume_name, data_dir, res_dir):
    # basic PUTs and GETs of fds-src cpp files
    smoke_ds0 = FdsDataSet(volume_name, data_dir, res_dir, '*.cpp')
    smoke0 = CopyS3Dir(smoke_ds0)

    smoke0.runTest()
    smoke0.resetTest()

    smoke0.exitOnError()

def stressIO(volume_name, data_dir, res_dir):

    # seq write, then seq read
    smoke_ds1 = FdsDataSet(volume_name, data_dir, res_dir, '*')
    smoke1 = CopyS3Dir(smoke_ds1)

    smoke1.runTest()
    smoke1.resetTest()

    smoke1.exitOnError()

    # write from in burst of 10
    smoke_ds2 = FdsDataSet(volume_name, data_dir, res_dir, '*')
    smoke2 = CopyS3Dir(smoke_ds2)
    smoke2.runTest(10)
    smoke2.exitOnError()

    # write from in burst of 10-5 (W-R)
    smoke_ds3 = FdsDataSet(volume_name, data_dir, res_dir, '*')
    smoke3 = CopyS3Dir_PatternRW(smoke_ds3)
    smoke3.runTest(10, 5)
    smoke3.exitOnError()

    # write from in burst of 10-3 (W-R)
    smoke_ds4 = FdsDataSet(volume_name, data_dir, res_dir, '*')
    smoke4 = CopyS3Dir_PatternRW(smoke_ds4)
    smoke4.runTest(10, 3)
    smoke4.exitOnError()

    # write from in burst of 1-1 (W-R)
    smoke_ds5 = FdsDataSet(volume_name, data_dir, res_dir, '*')
    smoke5 = CopyS3Dir_PatternRW(smoke_ds5)
    smoke5.runTest(1, 1)
    smoke5.exitOnError()

def blobOverwrite(volume_name, data_dir, res_dir):

    # seq write, then seq read
    smoke_ds1 = FdsDataSet(volume_name, data_dir, res_dir, '*')
    smoke1 = CopyS3Dir_Overwrite(smoke_ds1)
    smoke1.runTest()
    smoke1.exitOnError()

def stressIO_GET(volume_name, data_dir, res_dir):
    # seq write, then seq read
    smoke_ds1 = FdsDataSet(volume_name, data_dir, res_dir, '*')
    smoke1 = CopyS3Dir_GET(smoke_ds1)

    smoke1.runTestAll()
    smoke1.resetTest()

    smoke1.exitOnError()

def startIOHelper(volume_name, data_set_dir, res_dir, loop):
    i = 0
    while i < loop or loop == 0:
        # basic PUTs and GETs of fds-src cpp files
        preCommit(volume_name, data_set_dir, res_dir)

        # stress I/O
        stressIO(volume_name, data_set_dir, res_dir)

        # blob over-write
        blobOverwrite(volume_name, data_set_dir, res_dir)
        i += 1

def startIOHelperThrd(volume_name, data_set_dir, res_dir, loop):
    sys.stdout = open('/tmp/smoke_io-' + str(os.getpid()) + ".out", "w")
    startIOHelper(volume_name, data_set_dir, res_dir, loop)

def startIO(volume_name, data_set_dir, res_dir, async, loop):
    if async == True:
        startIOProcess = Process(target=startIOHelperThrd, args=(volume_name, data_set_dir, res_dir, loop))
        startIOProcess.start()
    else:
        startIOHelper(volume_name, data_set_dir, res_dir, loop)
        startIOProcess = None
    return startIOProcess

# -------------------------------------------------------------------

def startIO_GETHelper(volume_name, data_set_dir, res_dir, loop):
    i = 0
    while i < loop or loop == 0:
        # stress I/O
        stressIO_GET(volume_name, data_set_dir, res_dir)
        i += 1

def startIO_GETHelperThrd(volume_name, data_set_dir, res_dir, loop):
    sys.stdout = open('/tmp/smoke_io-' + str(os.getpid()) + ".out", "w")
    startIO_GETHelper(volume_name, data_set_dir, res_dir, loop)

def startIO_GET(volume_name, data_set_dir, res_dir, async, loop):
    if async == True:
        startIOProcess = Process(target=startIO_GETHelperThrd, args=(volume_name, data_set_dir, loop))
        startIOProcess.start()
    else:
        startIO_GETHelper(volume_name, data_set_dir, res_dir, loop)
        startIOProcess = None
    return startIOProcess

def waitIODone(p):
    if p != None:
        while p.is_alive():
            print "Waiting for process to complete: "
            p.join(60)
        if p.exitcode != 0:
            print "Process terminated with Failure: "
            sys.exit(p.exitcode)
        print "Async Process terminated: "
    print "Sync Process terminated: "
#########################################################################################

def exitTest(env, shutdown):
    print "Total PUTs: ", env.total_put
    print "Total GETs: ", env.total_get
    print "Test Passed, cleaning up..."
    if shutdown:
        env.cleanup()
    sys.exit(0)

data_set_dir = None

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Start FDS Processes...')
    parser.add_argument('--cfg_file', default='./test/smoke_test.cfg', help='Set FDS test cfg')
    parser.add_argument('--start_sys', default='true', help='Bringup cluster [true]')
    parser.add_argument('--smoke_test', default='false', help='Run full smoke test [false]')
    parser.add_argument('--data_set', default='/smoke_test', help='Smoke test dataset [/smoke_test]')
    parser.add_argument('--verbose', default='false', help ='Print verbose [false]')
    parser.add_argument('--async', default='true', help ='Run I/O Async [true]')
    parser.add_argument('--debug', default='false', help ='pdb debug on [false]')
    parser.add_argument('--shutdown', default='true', help ='Shutdown/cleanup system after test passed [true]')
    args = parser.parse_args()

    cfgFile = args.cfg_file
    smokeTest = args.smoke_test
    data_set_dir = args.data_set
    verbose = args.verbose
    debug = args.debug
    start_sys = args.start_sys
    if args.async == 'true':
        async_io = True
    else:
        async_io = False
    loop = 1

    if args.shutdown == 'true':
        shutdown = True
    else:
        shutdown = False

    #
    # Setup lab key access
    #
    # subprocess.call(['test/copy_key.sh'])

    #
    # Load the configuration files
    #
    bu = ServiceConfig.TestBringUp(cfgFile, verbose, debug)
    bu.loadCfg()
    env = FdsEnv(bu.getCfgField('node1', 'fds_root'))

    #
    # Bring up the cluster
    #
    # bringupCluster(env, bu, cfgFile, verbose, debug)
    if start_sys == 'true':
        bringupCluster(env, bu, cfgFile, verbose, debug)
        time.sleep(2)
    # bringupClusterCLI(env, bu, cfgFile, verbose, debug)

    if args.smoke_test == 'false':
        preCommit('volume_smoke1', env.srcdir, '/tmp/pre_commit')
        exitTest(env, shutdown)

    #
    # Start Smoke Test
    # Start sync-IO
    # Start async-IO
    #
    p1 = startIO('volume_smoke1', data_set_dir, '/tmp/res1', async_io, loop)
    p2 = startIO('volume_smoke2', data_set_dir, '/tmp/res2', async_io, loop)
    p3 = startIO('volume_smoke3', data_set_dir, '/tmp/res3', async_io, loop)
    p4 = startIO('volume_smoke4', data_set_dir, '/tmp/res4', async_io, loop)
    # p = startIO_GET('volume_smoke1', data_set_dir, True, loop)


    #
    # Bring up more node/delete node test
    #
# bringupNode(env, bu, cfgFile, verbose, debug, 'node2')
#    bringupNode(env, bu, cfgFile, verbose, debug, 'node3')
#    bringupNode(env, bu, cfgFile, verbose, debug, 'node4')

    #
    # Wait for async I/O to complete
    #
    waitIODone(p1)
    waitIODone(p2)
    waitIODone(p3)
    waitIODone(p4)
    exitTest(env, shutdown)

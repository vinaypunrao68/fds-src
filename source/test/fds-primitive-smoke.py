#!/usr/bin/env python

import os, sys
import argparse
import subprocess
import pdb

class FdsEnv:
    def __init__(self):
        self.env_cdir      = os.getcwd()
        self.env_fdsroot   = ''
        self.env_exitOnErr = True
        self.total_put     = 0
        self.total_get     = 0

        tmp_dir = self.env_cdir
        while tmp_dir != "/":
            if os.path.exists(tmp_dir + '/Build/mk-scripts'):
                self.env_fdsroot = tmp_dir
                break
            tmp_dir = os.path.dirname(tmp_dir)

        if self.env_fdsroot == "":
            print "Can't determine FDS root from the current dir ", self.env_cdir
            sys.exit(1)

    def shut_down(self):
        subprocess.call(['pkill', '-9', 'AMAgent'])
        subprocess.call(['pkill', '-9', 'Mgr'])

    def cleanup(self):
        self.shut_down()
        os.chdir(self.env_fdsroot)
        subprocess.call(['test/cleanup.sh'])

class FdsSetupEnv:
    def __init__(self, env, fds_data_path):
        try:
            os.mkdir(fds_data_path)
            os.mkdir(fds_data_path + '/hdd')
            os.mkdir(fds_data_path + '/ssd')
        except:
            pass
        subprocess.call(['cp', '-rf', env.env_fdsroot + '/config/etc', fds_data_path])

class FdsDataSet:
    def __init__(self, bucket='abc', data_dir='', file_ext='*', dest_dir='/tmp'):
        self.ds_bucket   = bucket
        self.ds_data_dir = data_dir
        self.ds_file_ext = file_ext
        self.ds_dest_dir = dest_dir

        if not self.ds_data_dir or self.ds_data_dir == '':
            self.ds_data_dir = os.getcwd()

        if not self.ds_dest_dir or self.ds_dest_dir == '':
            self.ds_dest_dir = '/tmp'
       
class CopyS3Dir:
    def __init__(self, env, dataset):
        self.env        = env
        self.ds         = dataset
        self.verbose    = False
        self.bucket     = dataset.ds_bucket 
        self.files_list = []
        self.cur_put    = 0
        self.cur_get    = 0
        self.perr_cnt   = 0
        self.gerr_cnt   = 0
        self.gerr_chk   = 0
        self.bucket_url = 'http://localhost:8000/' + self.bucket

        os.chdir(self.ds.ds_data_dir)
        files = subprocess.Popen(
            ['find', '.', '-name', self.ds.ds_file_ext, '-print'],
            stdout=subprocess.PIPE
        ).stdout
        for rec in files:
            rec = rec.rstrip('\n')
            self.files_list.append(rec)

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
            obj = put.replace("/", "_")
            self.printDebug("curl POST: " + put + " -> " + obj)
            ret = subprocess.call(['curl', '-X', 'POST', '--data-binary',
                    '@' + put, self.bucket_url + '/' + obj])
            if ret != 0:
                err = err + 1
                print "curl error: " + put + ": " + obj
                if self.env.env_exitOnErr == True:
                    sys.exit(1)

            if (cnt % 10) == 0:
                print "Put ", cnt + self.cur_put , " objects... errors: [", err, "]"
            if cnt == burst:
                break

        print "Put ", cnt + self.cur_put, " objects... errors: [", err, "]"
        self.cur_put  += cnt
        self.perr_cnt += err
        self.env.total_put += cnt

    def getTest(self, burst=0):
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        chk = 0
        # print self.files_list
        # print "\n"
        if burst <= 0:
            burst = len(self.files_list)
        for get in self.files_list[self.cur_get:]:
            cnt = cnt + 1
            obj = get.replace("/", "_");
            tmp = self.ds.ds_dest_dir + '/' + obj
            self.printDebug("curl GET: " + obj + " -> " + tmp)
            ret = subprocess.call(['curl', '-s' '1' ,
                                   self.bucket_url + '/' + obj, '-o', tmp])
            if ret != 0:
                err = err + 1
                print "curl error: " + get + ": " + obj + " -> " + tmp
                if self.env.env_exitOnErr == True:
                    sys.exit(1)

            ret = subprocess.call(['diff', get, tmp])
            if ret != 0:
                chk = chk + 1
                print "Get ", get, " -> ", tmp, ": ", cnt,  \
                      " objects... errors: [", err, "], verify failed [", chk, "]"
                if self.env.env_exitOnErr == True:
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
        self.env.total_get += cnt

    def exitOnError(self):
        if self.perr_cnt != 0 or self.gerr_cnt != 0 or self.gerr_chk != 0:
            print "Test Failed: put errors: [", self.perr_cnt,  \
                  "], get errors: [", self.gerr_cnt,            \
                  "], verify failed [", self.err_chk, "]"
            sys.exit(1)

        
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
        wr_file = self.files_list[0]
        for put in self.files_list:
            cnt = cnt + 1
            obj = put.replace("/", "_")
            self.printDebug("curl POST: " + put + " -> " + obj)
            ret = subprocess.call(['curl', '-X', 'POST', '--data-binary',
                    '@' + wr_file, self.bucket_url + '/' + obj])
            if ret != 0:
                err = err + 1
                print "curl error: " + put + ": " + obj
                if self.env.env_exitOnErr == True:
                    sys.exit(1)

            if (cnt % 10) == 0:
                print "Put ", cnt + self.cur_put , " objects... errors: [", err, "]"

        print "Put ", cnt + self.cur_put, " objects... errors: [", err, "]"
        self.cur_put  += cnt
        self.perr_cnt += err
        self.env.total_put += cnt

    def getTest(self, burst=0):
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        chk = 0
        rd_file = self.files_list[0]
        for get in self.files_list:
            cnt = cnt + 1
            obj = get.replace("/", "_");
            tmp = self.ds.ds_dest_dir + '/' + obj
            self.printDebug("curl GET: " + obj + " -> " + tmp)
            ret = subprocess.call(['curl', '-s' '1' ,
                                   self.bucket_url + '/' + obj, '-o', tmp])
            if ret != 0:
                err = err + 1
                print "curl error: " + get + ": " + obj + " -> " + tmp
                if self.env.env_exitOnErr == True:
                    sys.exit(1)

            ret = subprocess.call(['diff', rd_file, tmp])
            if ret != 0:
                chk = chk + 1
                print "Get ", get, " -> ", tmp, ": ", cnt,  \
                      " objects... errors: [", err, "], verify failed [", chk, "]"
                if self.env.env_exitOnErr == True:
                    sys.exit(1)
            else:
# print tmp, " : ", rd_file
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
        self.env.total_get += cnt


def preCommit(env):
    # basic PUTs and GETs of fds-src cpp files
    smoke_ds0 = FdsDataSet('smoke_vol0', env.env_fdsroot, '*.cpp')
    smoke0 = CopyS3Dir(env, smoke_ds0)
    smoke0.runTest()
    smoke0.exitOnError()

def stressIO(env):

    # seq write, then seq read
    smoke_ds1 = FdsDataSet('smoke_vol1', data_set_dir, '*.jpg')
    smoke1 = CopyS3Dir(env, smoke_ds1)
    smoke1.runTest()
    smoke1.exitOnError()

    # write from in burst of 10
    smoke_ds2 = FdsDataSet('smoke_vol2', data_set_dir, '*.jpg')
    smoke2 = CopyS3Dir(env, smoke_ds2)
    smoke2.runTest(10)
    smoke2.exitOnError()

    # write from in burst of 10-5 (W-R)
    smoke_ds3 = FdsDataSet('smoke_vol3', data_set_dir, '*.jpg')
    smoke3 = CopyS3Dir_PatternRW(env, smoke_ds3)
    smoke3.runTest(10, 5)
    smoke3.exitOnError()

    # write from in burst of 10-3 (W-R)
    smoke_ds4 = FdsDataSet('smoke_vol4', data_set_dir, '*.jpg')
    smoke4 = CopyS3Dir_PatternRW(env, smoke_ds4)
    smoke4.runTest(10, 3)
    smoke4.exitOnError()

    # write from in burst of 1-1 (W-R)
    smoke_ds5 = FdsDataSet('smoke_vol5', data_set_dir, '*.jpg')
    smoke5 = CopyS3Dir_PatternRW(env, smoke_ds5)
    smoke5.runTest(1, 1)
    smoke5.exitOnError()

    # write from ~/temp/demo_data folder in burst of 10
#    demo_ds = FdsDataSet('volume6', data_set_dir, '*.jpg')
#    demo = CopyS3Dir(env, demo_ds)
#    demo.runTest()

#    subprocess.call(['sleep', '200']);

def migration(env):

    # seq write, then seq read
    smoke_ds1 = FdsDataSet('checker', data_set_dir, '*.jpg')
    smoke1 = CopyS3Dir(env, smoke_ds1)
    smoke1.runTest()
    smoke1.exitOnError()

    # testing migration
    os.chdir(env.env_fdsroot + '/Build/linux-x86_64.debug/node2')
    print "Starting SM on node2...."
    subprocess.Popen(['./StorMgr', '--fds-root', args.root + '/node2', '--fds.sm.data_port=7911',
                      '--fds.sm.control_port=6911', '--fds.sm.prefix=node2_', '--fds.sm.test_mode=false',
                      '--fds.sm.log_severity=0', '--fds.sm.om_ip=127.0.0.1', '--fds.sm.migration.port=8610', '--fds.sm.id=sm2'],
                     stderr=subprocess.STDOUT)
    subprocess.call(['sleep', '60'])

    # call checker

def blobOverwrite(env):

    # seq write, then seq read
    smoke_ds1 = FdsDataSet('smoke_vol1', data_set_dir, '*.jpg')
    smoke1 = CopyS3Dir_Overwrite(env, smoke_ds1)
    smoke1.runTest()
    smoke1.exitOnError()

data_set_dir = '/tmp/smoke_test'

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Start FDS Processes...')
    parser.add_argument('--root', default='/fds', help = 'Set FDS Root')
    parser.add_argument('--smoke_test', default='false', help = 'Run full smoke test')
    parser.add_argument('--data_set', default='/tmp/smoke_test', help = 'smoke test dataset')

    env  = FdsEnv()
    args = parser.parse_args()

    env.cleanup()
    print "Setting up private fds-root in ", args.root + '/node[1-4]'
    FdsSetupEnv(env, args.root + '/node1')
    FdsSetupEnv(env, args.root + '/node2')
    FdsSetupEnv(env, args.root + '/node3')
    FdsSetupEnv(env, args.root + '/node4')

    os.chdir(env.env_fdsroot + '/Build/linux-x86_64.debug/bin')
    print "Starting OM...."
    subprocess.Popen(['./orchMgr', '--fds-root', args.root + '/node1'],
                     stderr=subprocess.STDOUT)
    subprocess.call(['sleep', '1'])

    print "Starting SM on node1...."
    subprocess.Popen(['./StorMgr', '--fds-root', args.root + '/node1'],
                     stderr=subprocess.STDOUT)
    subprocess.call(['sleep', '1'])

    print "Starting DM on node1...."
    subprocess.Popen(['./DataMgr', '--fds-root', args.root + '/node1'],
                     stderr=subprocess.STDOUT)
    subprocess.call(['sleep', '1']);

    print "Starting AM on node1...."
    subprocess.Popen(['./AMAgent', '--fds-root', args.root + '/node1'],
                     stderr=subprocess.STDOUT)
    subprocess.call(['sleep', '3']);

    data_set_dir = args.data_set

    # basic PUTs and GETs of fds-src cpp files
    preCommit(env)

    if args.smoke_test == 'true':
        # stress I/O
        stressIO(env)

        # migration test
        # migration(env)

        # blob over-write
        blobOverwrite(env)

    # do shut_down clients on cleanup test run
    print "Total PUTs: ", env.total_put
    print "Total GETs: ", env.total_get
    print "Test Passed, cleaning up..."
    # env.shut_down()
    sys.exit(0)

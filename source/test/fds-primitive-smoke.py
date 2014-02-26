#!/usr/bin/env python

import os, sys
import argparse
import subprocess
import pdb

class FdsEnv:
    def __init__(self):
        self.env_cdir     = os.getcwd()
        self.env_fdsroot  = ''

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
    def __init__(self, bucket='abc', data_dir='.', file_ext='*'):
        self.ds_bucket   = bucket
        self.ds_data_dir = data_dir
        self.ds_file_ext = file_ext

        if not self.ds_data_dir or self.ds_data_dir == '':
            self.ds_data_dir = '.'

       
class CopyS3Dir:
    def __init__(self, env, dataset):
        self.env        = env
        self.ds         = dataset
        self.verbose    = True
        self.bucket     = dataset.ds_bucket 
        self.files_list = []
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

    def PutTest(self):
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        ret = subprocess.call(['curl', '-X' 'PUT', self.bucket_url])
        if ret != 0:
            print "Bucket create failed"
            sys.exit(1)

        for put in self.files_list:
            cnt = cnt + 1
            obj = put.replace("/", "_")
            ret = subprocess.call(['curl', '-X', 'POST', '--data-binary',
                    '@' + put, self.bucket_url + '/' + obj])

            if ret != 0:
                err = err + 1
                print "curl error: " + put + ": " + obj
            if (cnt % 10) == 0:
                print "Put ", cnt, " objects... errors: [", err, "]"

        print "Put ", cnt, " objects... errors: [", err, "]"
        self.perr_cnt += err

    def GetTest(self):
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        chk = 0
        print self.files_list
        print "\n"
        for get in self.files_list:
            cnt = cnt + 1
            obj = get.replace("/", "_");
            tmp = '/tmp/' + obj
            ret = subprocess.call(['curl', '-s' '1' ,
                                   self.bucket_url + '/' + obj, '-o', tmp])
            if ret != 0:
                err = err + 1
                print "curl error: " + get + ": " + obj + " -> " + tmp

            ret = subprocess.call(['diff', get, tmp])
            if ret != 0:
                chk = chk + 1
                print "Get ", get, " -> ", tmp, ": ", cnt, \
                      " objects... errors: [", err, "], verify failed [", chk, "]"
            else:
                subprocess.call(['rm', tmp])
            if (cnt % 10) == 0:
                print "Get ", cnt, \
                      " objects... errors: [", err, "], verify failed [", chk, "]"


        print "Get ", cnt, " objects... errors: [", err, "], verify failed [", chk, "]"
        self.gerr_cnt += err
        self.gerr_chk += chk

    def exitOnError(self):
        if self.perr_cnt != 0 or self.gerr_cnt != 0 or self.gerr_chk != 0:
            print "Test Failed: put errors: [", self.perr_cnt,  \
                  "], get errors: [", self.gerr_cnt,            \
                  "], verify failed [", self.err_chk, "]"
            sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Start FDS Processes...')
    parser.add_argument('--root', default='/fds', help = 'Set FDS Root')

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
    subprocess.Popen(['./orchMgr'], stderr=subprocess.STDOUT)
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
    subprocess.Popen(['./AMAgent'], stderr=subprocess.STDOUT)
    subprocess.call(['sleep', '3']);

    # basic PUTs and GETs
    dataset1 = FdsDataSet('abc', env.env_fdsroot, '*.cpp')
    smoke = CopyS3Dir(env, dataset1)
    smoke.PutTest()
    smoke.GetTest()

    # write from ~/temp/demo_data folder
    dataset2 = FdsDataSet('volume6', '/home/bao_pham/temp/demo_data', '*.jpg')
    demo = CopyS3Dir(env, dataset2)
    demo.PutTest()
    demo.GetTest()


#    subprocess.call(['sleep', '200']);


    # testing migration
#    os.chdir(env.env_fdsroot + '/Build/linux-x86_64.debug/node2')
#    print "Starting SM on node2...."
#    subprocess.Popen(['./StorMgr', '--fds-root', args.root + '/node2', '--fds.sm.data_port=7911',
#                      '--fds.sm.control_port=6911', '--fds.sm.prefix=node2_', '--fds.sm.test_mode=false',
#                      '--fds.sm.log_severity=0', '--fds.sm.om_ip=127.0.0.1', '--fds.sm.migration.port=8610', '--fds.sm.id=sm2'],
#                     stderr=subprocess.STDOUT)
#    subprocess.call(['sleep', '1'])


#    smoke.exitOnError()


    # add test cases from excell


    # how to generate data



    # do shut_down clients on cleanup test run
    print "Test Passed, cleaning up..."
    #env.shut_down()


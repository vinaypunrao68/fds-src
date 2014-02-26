#!/usr/bin/env python

import os, sys
import argparse
import subprocess

class FdsEnv:
    def __init__(self):
        self.env_cdir    = os.getcwd()
        self.env_fdsroot = ''

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
       
class FdsRootCopy:
    def __init__(self, env):
        self.env      = env
        self.cpp_list = []
        self.err_cnt  = 0
        self.err_chk  = 0

        os.chdir(self.env.env_fdsroot)
        cpp_files = subprocess.Popen(
            ['find', '.', '-name', '*.cpp', '-print'],
            stdout=subprocess.PIPE
        ).stdout
        for rec in cpp_files:
            rec = rec.rstrip('\n')
            self.cpp_list.append(rec)

    def PutTest(self):
        os.chdir(self.env.env_fdsroot)
        cnt = 0
        err = 0
        ret = subprocess.call(['curl', '-X' 'PUT', 'http://localhost:8000/abc'])
        if ret != 0:
            print "Bucket create failed"
            sys.exit(1)

        for put in self.cpp_list:
            cnt = cnt + 1
            obj = put.replace("/", "_")
            ret = subprocess.call(['curl', '-X', 'POST', '--data-binary',
                    '@' + put, 'http://localhost:8000/abc/' + obj])

            if ret != 0:
                print "curl error: " + put + ": " + obj
                err = err + 1
            if (cnt % 10) == 0:
                print "Put ", cnt, " objects... errors: [", err, "]"

        print "Put ", cnt, " objects... errors: [", err, "]"
        self.err_cnt += err

    def GetTest(self):
        os.chdir(self.env.env_fdsroot)
        cnt = 0
        err = 0
        chk = 0
        print self.cpp_list
        print "\n"
        for get in self.cpp_list:
            cnt = cnt + 1
            obj = get.replace("/", "_");
            tmp = '/tmp/' + obj
            ret = subprocess.call(['curl', '-s' '1' , 'http://localhost:8000/abc/' + obj, '-o', tmp])
            if ret != 0:
                print "curl error: " + get + ": " + obj + " -> " + tmp
                err = err + 1

            ret = subprocess.call(['diff', get, tmp])
            if ret != 0:
                chk = chk + 1
                print "Get ", get, " -> ", tmp, ": ", cnt, " objects... errors: [", err, "], verify failed [", chk, "]"
            else:
                subprocess.call(['rm', tmp])
            if (cnt % 10) == 0:
                print "Get ", cnt, " objects... errors: [", err, "], verify failed [", chk, "]"


        print "Get ", cnt, " objects... errors: [", err, "], verify failed [", chk, "]"
        self.err_cnt += err
        self.err_chk += chk

    def exitOnError(self):
        if self.err_cnt != 0 or self.err_chk != 0:
            print "Test Failed: errors: [", self.err_cnt, "], verify failed [", self.err_chk, "]"
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
    subprocess.Popen(['./AMAgent', '--fds-root', args.root + '/node1'],
                     stderr=subprocess.STDOUT)
    subprocess.call(['sleep', '3']);

    smoke = FdsRootCopy(env)
    smoke.PutTest()
    smoke.GetTest()
    smoke.exitOnError()

    # do shut_down clients on cleanup test run
    print "Test Passed, cleaning up..."
    env.shut_down()


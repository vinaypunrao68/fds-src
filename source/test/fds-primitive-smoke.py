#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
import os, sys
import inspect
import argparse
import subprocess
from multiprocessing import Process, Array
import time
import boto
from boto.s3.connection import OrdinaryCallingFormat
from boto.s3.key import Key
from boto.exception import S3CreateError

###
# Enumerations
#
class IOSTAT:
    PUT, GET, DEL = range(0,3)

class HTTPERROR:
    OK = 200
    NOT_FOUND = 404

REMOTE = False
CONFIG = None

##########################################################

###
# Fds test environment
#
class FdsEnv:
    def __init__(self, _root):
        self.env_cdir        = os.getcwd()
        self.srcdir          = ''
        self.env_root        = _root
        self.env_exit_on_err = True

        # assuming that this script is located in the test dir
        self.testdir  = os.path.dirname(
                            os.path.abspath(inspect.getfile(inspect.currentframe())))
        self.srcdir   = os.path.abspath(self.testdir + "/..")
        self.toolsdir = os.path.abspath(self.srcdir + "/tools")
        self.fdsctrl  = self.toolsdir + "/fds"
        self.cfgdir   = self.testdir + "/cfg"
        if CONFIG is not None:
            self.rem_cfg  = self.cfgdir + "/" + CONFIG
        else:
            self.rem_cfg  = self.cfgdir

        if self.srcdir == "":
            print "Can't determine FDS root from the current dir ", self.env_cdir
            sys.exit(1)

    def shut_down(self):
        if REMOTE:
            subprocess.call("python " + self.testdir + "/fds-tool.py -f " + self.rem_cfg + " -d", shell=True)
        else:
            subprocess.call([self.fdsctrl, 'stop', '--fds-root', self.env_root])

    def cleanup(self):
        self.shut_down()
        if REMOTE == False:
            subprocess.call([self.fdsctrl, 'clean', '--fds-root', self.env_root])
        else:
            subprocess.call("python " + self.testdir + "/fds-tool.py -f " + self.rem_cfg + " -c", shell=True)

###
# setup test environment, directories, etc
#
class FdsSetupNode:
    def __init__(self, fdsSrc, fds_data_path):
        try:
            os.mkdir(fds_data_path)
        except:
            pass
        subprocess.call(['cp', '-rf', fdsSrc + '/config/etc', fds_data_path])

###
# dataset and bucket information
#
class FdsDataSet:
    def __init__(self, am_ip='localhost', bucket='abc', data_dir='',
                 dest_dir='/tmp', file_ext='*', log_level=0):
        self.ds_am_ip    = am_ip
        self.ds_bucket   = bucket
        self.ds_data_dir = data_dir
        self.ds_file_ext = file_ext
        self.ds_dest_dir = dest_dir
        self.ds_log_level = log_level

        if not self.ds_data_dir or self.ds_data_dir == '':
            self.ds_data_dir = os.getcwd()

        if not self.ds_dest_dir or self.ds_dest_dir == '':
            self.ds_dest_dir = '/tmp'
        ret = subprocess.call(['mkdir', '-p', self.ds_dest_dir])

###
# test runner parameters
#
class FdsTestArgs:
    def __init__(self, volume_name, data_set_dir, result_dir, loop_cnt, async, am_ip, log_level):
        self.args_volume_name  = volume_name
        self.args_data_set_dir = data_set_dir
        self.args_result_dir   = result_dir
        self.args_loop_cnt     = loop_cnt
        self.args_async_io     = async
        self.args_am_ip        = am_ip
        self.log_level         = log_level

###
# global test stats
#
class FdsStats:
    def __init__(self):
        self.total_puts = 0
        self.total_gets = 0
        self.total_dels = 0

#########################################################################################
###
# put a directory and get it back
#
class CopyS3Dir:
    global_obj_prefix = 0
    def __init__(self, dataset):
        self.ds         = dataset
        self.verbose    = False
        self.bucket     = dataset.ds_bucket
        self.exit_on_err= True
        self.files_list = []
        self.cur_put    = 0
        self.cur_get    = 0
        self.perr_cnt   = 0
        self.gerr_cnt   = 0
        self.gerr_chk   = 0
        self.log_level  = int(dataset.ds_log_level)

        global c
        self.c = c

        self.obj_prefix = str(CopyS3Dir.global_obj_prefix)
        CopyS3Dir.global_obj_prefix += 1
        self.bucket_url = 'http://' + dataset.ds_am_ip + ':8000/' + self.bucket

        os.chdir(self.ds.ds_data_dir)
        files = subprocess.Popen(
            ['find', '.', '-name', self.ds.ds_file_ext, '-type', 'f', '-print'],
            stdout=subprocess.PIPE
        ).stdout
        for rec in files:
            rec = rec.rstrip('\n')
            self.files_list.append(rec)
        if len(self.files_list) == 0:
            print "Warning: data directory is empty"

    def reset_test(self):
        self.cur_put    = 0
        self.cur_get    = 0

    def log(self, msg):
        if self.log_level < 2:
            print msg

    def run_test(self, burst_cnt=0):
        self.create_bucket()
        total = len(self.files_list)
        if burst_cnt == 0 or burst_cnt > total:
            burst_cnt = total
        cnt = 0
        while cnt < total:
            self.put_test(burst_cnt)
            self.get_test(burst_cnt)
            cnt += burst_cnt

    def create_bucket(self):
        time.sleep(10)
        print 'Creating bucket: %s' % self.bucket
        try:
            self.b = c.create_bucket(self.bucket)
        except boto.exception.S3CreateError as s:
            pass
        except Exception as e:
            self.log(e)

    def delete_bucket(self):
        print 'Deleting bucket %s' % self.bucket
        try:
            for key in self.b.list():
                key.delete()
            self.b.delete()
        except Exception as e:
            self.log(e)

        time.sleep(1)

    def put_test(self, burst=0):
        global ioStats
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        if burst <= 0:
            burst = len(self.files_list)
        for put in self.files_list[self.cur_put:]:
            cnt = cnt + 1
            obj = self.obj_prefix + put.replace("/", "_")

            k = Key(self.c.get_bucket(self.bucket))
            k.key = obj
            k.set_contents_from_file(open(put, 'rb'))

            if (cnt % 10) == 0:
                self.log("Put " + str(cnt) + str(self.cur_put) + " objects... errors: ["+ str(err) + "]")
            if cnt == burst:
                break

        self.log("Put " + str(cnt) + str(self.cur_put) + " objects... errors: ["+ str(err) + "]")
        self.cur_put  += cnt
        self.perr_cnt += err
        ioStats[IOSTAT.PUT] += cnt

    def get_test(self, burst=0):
        global ioStats
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

            k = Key(self.c.get_bucket(self.bucket))
            k.key = obj
            k.get_contents_to_file(open(tmp, 'wb'))

            ret = subprocess.call(['diff', get, tmp])
            if ret != 0:
                chk = chk + 1
                self.log("Get "+ str(get)+ " -> "+ str(tmp)+ ": "+ str(cnt)+  \
                          " objects... errors: ["+ str(err)+ "], verify failed ["+str(chk)+ "]")
                if self.exit_on_err == True:
                    sys.exit(1)
            else:
                subprocess.call(['rm', tmp])
            if (cnt % 10) == 0:
                self.log("Get "+ str(cnt) + str(self.cur_get)+ \
                      " objects... errors: ["+ str(err)+ "], verify failed ["+str(chk)+ "]")
            if cnt == burst:
                break

        self.log("Get "+ str(cnt) + str(self.cur_get)+ " objects... errors: ["+ str(err)+ \
                  "], verify failed ["+ str(chk)+ "]")

        self.cur_get    += cnt
        self.gerr_cnt   += err
        self.gerr_chk   += chk
        ioStats[IOSTAT.GET] += cnt;

    def exit_on_error(self):
        if self.perr_cnt != 0 or self.gerr_cnt != 0 or self.gerr_chk != 0:
            print "Test Failed: put errors: [", self.perr_cnt,  \
                  "], get errors: [", self.gerr_cnt,            \
                  "], verify failed [", self.err_chk, "]"
            sys.exit(1)

###
# interleave put and get
#
class CopyS3Dir_Pattern(CopyS3Dir):
    def run_test(self, write=1, read=1):
        self.create_bucket()
        total = len(self.files_list)
        if write == 0 or write > total:
            write = total

        if read == 0 or read > total:
            read = total

        wr_cnt = 0
        rd_cnt = 0
        while wr_cnt < total and rd_cnt < total:
            self.put_test(write)
            self.get_test(read)
            wr_cnt += write
            rd_cnt += read

        if wr_cnt < total:
            self.put_test()
        if rd_cnt < total:
            self.get_test()

###
# overwrite existing objects
#
class CopyS3Dir_Overwrite(CopyS3Dir):
    def run_test(self):
        CopyS3Dir.run_test(self)

        self.put_test()
        self.get_test()

    def put_test(self, burst=0):
        global ioStats
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        for wr_file in self.files_list:
            for put in self.files_list:
                cnt = cnt + 1
                obj = self.obj_prefix + put.replace("/", "_")

                k = Key(self.c.get_bucket(self.bucket))
                k.key = obj
                k.set_contents_from_file(open(wr_file, 'rb'))

                if (cnt % 10) == 0:
                    self.log("Put "+ str(cnt) + str(self.cur_put)+ " objects... errors: ["+str(err)+ "]")

            self.log("Put "+ str(cnt) + str(self.cur_put)+ " objects... errors: ["+ str(err)+ "]")
            self.cur_put  += cnt
            self.perr_cnt += err
        ioStats[IOSTAT.PUT] += cnt

    def get_test(self, burst=0):
        global ioStats
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        chk = 0
        if len(self.files_list):
            rd_file = self.files_list[-1]
        for get in self.files_list:
            cnt = cnt + 1
            obj = self.obj_prefix + get.replace("/", "_");
            tmp = self.ds.ds_dest_dir + '/' + obj

            k = Key(self.c.get_bucket(self.bucket))
            k.key = obj
            k.get_contents_to_file(open(tmp, 'wb'))

            ret = subprocess.call(['diff', rd_file, tmp])
            if ret != 0:
                chk += 1
                self.log("Get "+ str(get)+ " -> "+ str(tmp)+ ": "+ str(cnt)+  \
                          " objects... errors: ["+ str(err)+ "], verify failed ["+ str(chk)+ "]")
                if self.exit_on_err == True:
                    sys.exit(1)
            else:
                subprocess.call(['rm', tmp])
            if (cnt % 10) == 0:
                self.log("Get "+str(cnt) + str(self.cur_get)+ \
                      " objects... errors: ["+ str(err)+ "], verify failed ["+ str(chk)+ "]")
            if cnt == burst:
                break

        self.log("Get "+ str(cnt) + str(self.cur_get)+ " objects... errors: ["+ str(err)+ \
                  "], verify failed ["+ str(chk)+ "]")
        self.cur_get    += cnt
        self.gerr_cnt   += err
        self.gerr_chk   += chk
        ioStats[IOSTAT.GET] += cnt

###
# put a directory, get, then delete
#
class DelS3Dir(CopyS3Dir):
    def __init__(self, dataset):
        CopyS3Dir.__init__(self, dataset)
        self.cur_del = 0

    def reset_test(self):
        CopyS3Dir.reset_test(self)
        self.cur_del = 0

    def run_test(self, burst_cnt=0):
        self.create_bucket()
        total = len(self.files_list)
        if burst_cnt == 0 or burst_cnt > total:
            burst_cnt = total
        cnt = 0
        while cnt < total:
            self.put_test(burst_cnt)
            self.get_test(burst_cnt)
            self.del_test(burst_cnt)
            cnt += burst_cnt

    def del_test(self, burst=0):
        global ioStats
        os.chdir(self.ds.ds_data_dir)
        cnt = 0
        err = 0
        if burst <= 0:
            burst = len(self.files_list)
        for delete in self.files_list[self.cur_del:]:
            cnt = cnt + 1
            obj = self.obj_prefix + delete.replace("/", "_")

            k = Key(self.c.get_bucket(self.bucket))
            k.key = obj

            #TODO: Temporary workaround until error codes are fixed for delete
            try:
                k.delete()
            except: pass

            # Should throw an exception here
            try:
                k.get_contents_as_string()
            except boto.exception.S3ResponseError as e:
                pass
            else:
                raise Exception('Should not be able to get contents from a deleted key!')

            if (cnt % 10) == 0:
                self.log("Del "+ str(cnt) + str(self.cur_del) + " objects... errors: ["+ str(err)+ "]")
            if cnt == burst:
                break

        self.log("Del "+ str(cnt) + str(self.cur_del)+ " objects... errors: ["+ str(err)+ "]")
        self.cur_del    += cnt
        self.perr_cnt   += err
        ioStats[IOSTAT.DEL] += cnt

#########################################################################################
###
# cluster bring up
#
def bringup_cluster(env, verbose, debug):
    env.cleanup()
    if REMOTE:
        print "python " + env.testdir + "/fds-tool.py -f " + env.rem_cfg + " -u"
        subprocess.call("python " + env.testdir + "/fds-tool.py -f " + env.rem_cfg + " -u", shell=True)
    else:
        root1 = env.env_root
        print "\nSetting up private fds-root in " + root1
        FdsSetupNode(env.srcdir, root1)

        os.chdir(env.srcdir + '/Build/linux-x86_64.debug/bin')

        print "\n\nStarting fds on ", root1
        subprocess.call([env.fdsctrl, 'start', '--fds-root', root1])
        os.chdir(env.srcdir)

###
# exit the test, shutdown if requested
#
def exit_test(env, shutdown):
    global ioStats

    print "Total PUTs: ", ioStats[IOSTAT.PUT]
    print "Total GETs: ", ioStats[IOSTAT.GET]
    print "Total DELs: ", ioStats[IOSTAT.DEL]
    print "Test Passed, cleaning up..."

    if shutdown:
        env.cleanup()
    sys.exit(0)
#########################################################################################
###
# pre-commit test
#
def testsuite_pre_commit(test_a):
    # basic PUTs and GETs of fds-src cpp files
    print 'Write, read, check ...'
    smoke_ds0 = FdsDataSet(test_a.args_am_ip,
                           test_a.args_volume_name,
                           test_a.args_data_set_dir,
                           test_a.args_result_dir,
                           '*.cpp',
                           test_a.log_level)

    smoke0 = CopyS3Dir(smoke_ds0)
    smoke0.run_test()
    smoke0.reset_test()
    smoke0.exit_on_error()

    # seq write, then seq read
    print 'Write, read, delete, check ...'
    smoke_ds1 = FdsDataSet(test_a.args_am_ip,
                           test_a.args_volume_name,
                           test_a.args_data_set_dir,
                           test_a.args_result_dir,
                           '*.cpp',
                           test_a.log_level)
#smoke1 = CopyS3Dir_Overwrite(smoke_ds1)
#    smoke1.run_test()
#    smoke1.exit_on_error()

    smoke1 = DelS3Dir(smoke_ds0)
    smoke1.run_test()
    smoke1.reset_test()
    smoke1.exit_on_error()


###
# I/O stress test
#
def testsuite_stress_io(test_a):

    # seq write, then seq read
    smoke_ds1 = FdsDataSet(test_a.args_am_ip,
                           test_a.args_volume_name,
                           test_a.args_data_set_dir,
                           test_a.args_result_dir,
                           '*',
                           test_a.log_level)
    smoke1 = DelS3Dir(smoke_ds1)

    smoke1.run_test()
    smoke1.reset_test()
    smoke1.exit_on_error()

    # write from in burst of 10
    smoke_ds2 = FdsDataSet(test_a.args_am_ip,
                           test_a.args_volume_name,
                           test_a.args_data_set_dir,
                           test_a.args_result_dir,
                           '*',
                           test_a.log_level)
    smoke2 = DelS3Dir(smoke_ds2)
    smoke2.run_test(10)
    smoke2.exit_on_error()

    # write from in burst of 10-5 (W-R)
    smoke_ds3 = FdsDataSet(test_a.args_am_ip,
                           test_a.args_volume_name,
                           test_a.args_data_set_dir,
                           test_a.args_result_dir,
                           '*',
                           test_a.log_level)
    smoke3 = CopyS3Dir_Pattern(smoke_ds3)
    smoke3.run_test(10, 5)
    smoke3.exit_on_error()

    # write from in burst of 10-3 (W-R)
    smoke_ds4 = FdsDataSet(test_a.args_am_ip,
                           test_a.args_volume_name,
                           test_a.args_data_set_dir,
                           test_a.args_result_dir,
                           '*',
                           test_a.log_level)
    smoke4 = CopyS3Dir_Pattern(smoke_ds4)
    smoke4.run_test(10, 3)
    smoke4.exit_on_error()

    # write from in burst of 1-1 (W-R)
    smoke_ds5 = FdsDataSet(test_a.args_am_ip,
                           test_a.args_volume_name,
                           test_a.args_data_set_dir,
                           test_a.args_result_dir,
                           '*',
                           test_a.log_level)
    smoke5 = CopyS3Dir_Pattern(smoke_ds5)
    smoke5.run_test(1, 1)
    smoke5.exit_on_error()

###
# overwrite test
#
def testsuite_blob_overwrite(test_a):

    # seq write, then seq read
    smoke_ds1 = FdsDataSet(test_a.args_am_ip,
                           test_a.args_volume_name,
                           test_a.args_data_set_dir,
                           test_a.args_result_dir,
                           '*',
                           test_a.log_level)
    smoke1 = CopyS3Dir_Overwrite(smoke_ds1)
    smoke1.run_test()
    smoke1.exit_on_error()

###
# negative test
#
#def testsuite_neg_test(test_a):
#    print "Negative test..."
#    print "WIN-267, curl PUT object with invalid filename crashed AM, expect non zero"
#    print "WIN-235, list bucket on non-existing volume"


###
# helper function to start I/O
#
def start_io_helper(test_a):
    i = 0
    loop = test_a.args_loop_cnt
    while i < loop or loop == 0:
        # stress I/O
        testsuite_stress_io(test_a)

        # blob over-write
        testsuite_blob_overwrite(test_a)
        i += 1

###
#  spawn process forI/O
#
def start_io_helper_thrd(test_a):
    global std_output
    if std_output == 'no':
        sys.stdout = open('/tmp/smoke_io-' + str(os.getpid()) + ".out", "w")
    start_io_helper(test_a)

###
# main I/O function
#
def start_io_main(test_a):
    if test_a.args_async_io == True:
        startIOProcess = Process(target=start_io_helper_thrd, args=(test_a,))
        startIOProcess.start()
    else:
        start_io_helper(test_a)
        startIOProcess = None
    return startIOProcess

###
# wait for I/O completion
#
def wait_io_done(p):
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

data_set_dir = None

# Create a threadsafe object for processes to share IO stats data through
global ioStats
ioStats = Array('i', range(3))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Start FDS Processes...')
    parser.add_argument('--cfg_file', default='./test/smoke_test.cfg',
                        help='Set FDS test cfg')
    parser.add_argument('--up', default='true',
                        help='Bringup system [true]')
    parser.add_argument('--down', default='true',
                        help='Shutdown/cleanup system after passed test run [true]')
    parser.add_argument('--smoke_test', default='false',
                        help='Run full smoke test [false]')
    parser.add_argument('--fds_root', default='/fds',
                        help='fds-root path [/fds')
    parser.add_argument('--vol_prefix', default='smoke',
                        help='prefix for bucket name')
    parser.add_argument('--data_set', default='/smoke_test',
                        help='Smoke test dataset [/smoke_test]')
    parser.add_argument('--thread', default=4,
                        help='Number of I/O threads')
    parser.add_argument('--loop_cnt', default=1,
                        help='Number of iterations per test suite to run')
    parser.add_argument('--async', default='true',
                        help='Run I/O Async [true]')
    parser.add_argument('--am_ip', default='localhost',
                        help='AM IP address [localhost')
    parser.add_argument('--verbose', default='false',
                        help='Print verbose [false]')
    parser.add_argument('--debug', default='false',
                        help ='pdb debug on [false]')
    parser.add_argument('--log_level', default=0,
                        help ='Sets verbosity of output.  Most verbose is 0, least verbose is 2')
    parser.add_argument('--std_output', default='yes',
                        help='Whether or not the script should print to stdout or to log files [no]')
    parser.add_argument('--config', default=None,
                        help='Name of config file to use (in test/cfg) for remote bringup')
    args = parser.parse_args()

    cfgFile      = args.cfg_file
    smokeTest    = args.smoke_test
    fds_root     = args.fds_root
    vol_prefix   = args.vol_prefix
    data_set_dir = args.data_set
    verbose      = args.verbose
    debug        = args.debug
    start_sys    = args.up
    thread_cnt   = args.thread
    loop_cnt     = args.loop_cnt
    am_ip        = args.am_ip
    log_level    = args.log_level
    config       = args.config

    global std_output
    std_output   = args.std_output;

    if args.async == 'true':
        async_io = True
    else:
        async_io = False

    if args.down == 'true':
        shutdown = True
    else:
        shutdown = False

    ###
    # TODO:
    #   - testsuite_stress_io should preserve CopyS3Dir test, not replace it with DelS3Dir
    #   - pass in the work-load that you want to run, default to all.
    #       (CopyS3Dir, DelS3Dir, etc)
    #   - add class for testing volume create/delete/list
    #   - improve over-write, 1) overwrite with same content
    #                         2) with smaller size content
    #                         3) larger size content
    #                         4) zero content
    #

    ###
    # bring up the cluster
    #
    # Running remotely
    #if am_ip != 'localhost':
    #    REMOTE = True
    #    print 'Running smoke test remotely on IP %s' % am_ip
    if config is not None:
        global CONFIG
        assert(config != '')
        REMOTE = True
        CONFIG = config

    ###
    # load the configuration file and build test environment
    #
    env = FdsEnv(fds_root)

    if start_sys == 'true':
        bringup_cluster(env, verbose, debug)
        time.sleep(2)

    # Connect to S3 interface with Boto
    global c
    c = boto.connect_s3(
        aws_secret_access_key='blablabla',
        aws_access_key_id='kekekekekek',
        host=am_ip,
        port=8000,
        is_secure=False,
        calling_format=boto.s3.connection.OrdinaryCallingFormat()
    )

    ###
    # run make precheckin and exit
    #
    if args.smoke_test == 'false':
        args = FdsTestArgs(vol_prefix + '_volume0',             # vol name
                           env.srcdir,                          # data set dir
                           '/tmp/pre_commit',                   # result dir
                           1,                                   # loop count
                           False,                               # async-ip
                           am_ip,                               # am ip address
                           log_level)

        testsuite_pre_commit(args)
        exit_test(env, shutdown)

    ###
    # start IO
    #
    thr = 0
    assert(thread_cnt >= 0)
    process_list = []

    # Check to see if data set dir exists
    if os.path.isdir(data_set_dir) == False:
        print '[Data Set Directory] %s does not exist, using %s instead.' % (data_set_dir, env.testdir)
        data_set_dir = env.testdir

    while thr < int(thread_cnt):
        args = FdsTestArgs(vol_prefix + '_volume' + str(thr),   # vol name
                           data_set_dir,                        # data set dir
                           '/tmp/res' + str(thr),               # result dir
                           loop_cnt,                            # loop count
                           async_io,                            # async-io
                           am_ip,                               # am ip address
                           log_level)
        p1 = start_io_main(args)
        process_list.append(p1)
        thr += 1
        time.sleep(10)

    ###
    # bring up more node/delete node test
    #

    ###
    # wait for async I/O to complete
    #
    for p1 in process_list:
        wait_io_done(p1)

    # Close S3 connection with boto
    c.close()

    ###
    # clean exit
    #
    exit_test(env, shutdown)

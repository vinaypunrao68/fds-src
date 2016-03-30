#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# Unit tests should be run with every refactoring. Minimal execution time
# is critical, otherwise developers will not run with every refactoring.
# Every unit test must pass on a single developer machine, without using
# a deployment.

# A unit test should not cross process boundaries. This means no dependency
# on IPC, databases, or the network. If the test needs to depend on one of
# those, put the test into run-integration-tests.py instead.

import os, sys
import inspect
import argparse
import subprocess
import re
from multiprocessing import Process, Array
import time

def about_to_quit(start_time):
    '''
    Arguments:
    ----------
    start_time :
    '''
    print("--- duration wall clock: %.3f seconds ---" % (time.time() - start_time))
    return

if __name__ == "__main__":
    start_time = time.time()
    parser = argparse.ArgumentParser(description='Start FDS Processes...')
    parser.add_argument('--cfg_file', default='test.cfg',
                        help='???')
    args = parser.parse_args()

    testdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    files_path = os.path.abspath(testdir + "/../source/cit")

    # Run C++ unit tests
    failure_count = 0
    os.chdir(testdir + "/../source/")
    files = open(files_path + "/gtest_ut")
    for rec in files:
        rec = rec.rstrip('\n')
        params = rec.split(' ', 1)
        bin_name = params.pop(0)
        args = ""
        if len(params) > 0:
            args += params.pop(0)
        args += ' --gtest_output=xml:' + files_path + '/' + bin_name + '.xml'
        bin_name += (" " + args)
        print "Running:", bin_name
        retcode = subprocess.call(bin_name, shell=True)
        if retcode != 0:
            failure_count += 1
            print "C++ Unit test ERROR returned a non-zero return {}, test: {}.".format(retcode, bin_name)

    if failure_count > 0:
        print "C++ Unit test failure count:  %d" %(failure_count)
        about_to_quit(start_time)
        exit (failure_count)

    # Run Java unit tests
    os.chdir(testdir + "/../source/java")
    cmd = ["make", "test"]
    status = subprocess.call(cmd)

    if status > 0:
        print "Java Unit test failure"
        about_to_quit(start_time)
        exit (status)

    about_to_quit(start_time)

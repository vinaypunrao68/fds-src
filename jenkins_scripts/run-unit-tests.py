#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
import os, sys
import inspect
import argparse
import subprocess
import re
from multiprocessing import Process, Array
import time

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Start FDS Processes...')
    parser.add_argument('--cfg_file', default='test.cfg',
                        help='???')
    args = parser.parse_args()

    testdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    files_path = os.path.abspath(testdir + "/../source/cit")

    # Run C++ unit tests
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
        subprocess.call(bin_name, shell=True)

    # Run Java unit tests
    os.chdir(testdir + "/../source/java")
    cmd = ["make", "test"]
    subprocess.call(cmd)

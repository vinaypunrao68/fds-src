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
        bin_name = re.split("/", rec)
        xml_path='--gtest_output=xml:' + files_path + '/' + bin_name[-1] + '.xml'
        print "Running:", rec, xml_path
        subprocess.call([rec, xml_path])

    # Run Java unit tests
    os.chdir(testdir + "/../source/java")
    cmd = ["make", "test"]
    subprocess.call(cmd)

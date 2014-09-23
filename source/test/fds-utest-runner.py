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
    build_test_bin = os.path.abspath(testdir + "/" + "../Build/linux-x86_64.debug/tests")

    os.chdir(build_test_bin)
    files = subprocess.Popen(
        ['find', '.', '-type', 'f', '-print'],
        stdout=subprocess.PIPE
    ).stdout

    for rec in files:
        rec = rec.rstrip('\n')
        if re.match("./utest_", rec):
            print "FOUND: ", rec
            xml_path='--gtest_output="xml:' + rec.lstrip('./') + '.xml"'
            print xml_path
            subprocess.call([rec.lstrip('./'), xml_path])

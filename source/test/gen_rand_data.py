#!/usr/bin/env python
import sys
import types
import random
import pdb
import argparse
import subprocess
import os
import string
import time
import optparse

if __name__ == '__main__':
    #
    # Parse command line options
    #
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option("-f", "--file_name", dest="file_name",
                      default="rand_out_", help="Filename prefix")
    parser.add_option("-m", "--min", dest="min_size",
                      default=100, help="Min file size")
    parser.add_option("-M", "--max", dest="max_size",
                      default=5242880, help="Max file size")
    parser.add_option("-n", "--count", dest="files_count",
                      default=1000, help="Total files to generate")
    parser.add_option("-v", "--verbose", dest="verbose",
                      default=False, help="enable verbosity")
    (options, args) = parser.parse_args()
    file_name = options.file_name
    min_size = options.min_size
    max_size = options.max_size
    verbose = options.verbose
    files_count = options.files_count

    #
    # dd out the files from urandom
    #
    i = 0
    while i < files_count:
        size = 'count=' + str(random.randint(min_size, max_size))
        num = str(i).zfill(4)
        out_file = file_name + num
        out_file = 'of=' + file_name + num
        print 'dd', 'if=/dev/urandom', out_file, 'bs=1', size
        subprocess.call(['dd', 'if=/dev/urandom', out_file, 'bs=1', size])
        i += 1

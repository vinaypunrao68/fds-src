#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import sys
import subprocess
import logging
import optparse

def setup_logger(file=None, level=logging.INFO):
    """
    Sets up logger
    """
    log = logging.getLogger()
    log.setLevel(level)
    handler = logging.StreamHandler(file)
    handler.setLevel(level)
    log.addHandler(handler)
    return log

def parse_fdscluster_args():
    """
    Parses normal fds cluster arguments
    """
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option('-f', '--file', dest = 'config_file',
                      help = 'configuration file (e.g. sample.cfg)', metavar = 'FILE')
    parser.add_option('-v', '--verbose', action = 'store_true', dest = 'verbose',
                      help = 'enable verbosity')
    (options, args) = parser.parse_args()
    if options.config_file is None:
        print "You need to pass config file"
        sys.exit(1)
    return options, args

def run(prog_path, args = '', wait_time_sec = 0):
    """
    Runs the program at prog_path with args as arguments
    TODO(Rao): Don't ignore wait_time_sec
    """
    arglist = []
    arglist.append(prog_path)
    arglist.extend(args.split())
    output = subprocess.check_output(arglist)
    print output

def run_async(prog_path, args):
    """
    TODO(Rao): Implement this.  This should return an
    instance of ProcessTracker, which we can use for tracking
    the spawned program
    """
    pass
#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import subprocess
import logging

def setup_logger(file=None, level=logging.INFO):
    log = logging.getLogger()
    log.setLevel(level)
    handler = logging.StreamHandler(file)
    handler.setLevel(level)
    log.addHandler(handler)
    return log

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
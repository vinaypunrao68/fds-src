#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import sys
import subprocess
import logging
import optparse
import types

class Output:
    def __init__(self):
        self.stdout = None
        self.stderr = None
        self.exitcode = -1

    def wasSuccess(self):
        return self.exitcode == 0

    def hasError(self):
        return self.stderr != None and len(self.stderr) > 0

    def hasOutput(self):
        return self.stdout != None and len(self.stdout) > 0

def setup_logger(file=None, level=logging.INFO):
    """
    Sets up logger
    """
    log = logging.getLogger()
    log.setLevel(level)
    if file is None:
        handler = logging.StreamHandler()
    else:
       handler = logging.FileHandler(file) 
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

def run(cmd, wait=True):
    """
    Runs the program at prog_path with args as arguments
    TODO(Rao): Don't ignore wait_time_sec
    """
    shell = False
    if type(cmd) == types.StringType:
        shell=True

    output = Output()

    p = subprocess.Popen(cmd, shell=shell, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)

    if p.stdout:
        output.stdout = p.stdout.read()

    if p.stderr:
        output.stderr = p.stderr.read()

    if wait:
        p.wait()
        output.exitcode = p.returncode

    return output

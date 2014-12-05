#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import os
import subprocess
import sys
import time

def cpu_count():
    '''
    Returns the number of CPUs in the system
    '''
    num_cpu = 1
    if sys.platform == 'win32':
        try:
            num_cpu = int(os.environ['NUMBER_OF_PROCESSORS'])
        except (ValueError, KeyError):
            pass
    elif sys.platform == 'darwin':
        try:
            num_cpu = int(os.popen('sysctl -n hw.ncpu').read())
        except ValueError:
            pass
    else:
        try:
            num_cpu = os.sysconf('SC_NPROCESSORS_ONLN')
        except (ValueError, OSError, AttributeError):
            pass
    return num_cpu

def exec_commands(cmds):
    '''
    '''
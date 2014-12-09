#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import os
import subprocess
import sys
import time
from __builtin__ import None
from subprocess import list2cmdline


def cpu_count():
    '''
    Returns the number of CPUs in the system

    Attributes:
    -----------
    None

    Returns:
    --------
    num_cpus : int
        the number of cpus in this machine
    '''
    num_cpus = 1
    if sys.platform == 'win32':
        try:
            num_cpus = int(os.environ['NUMBER_OF_PROCESSORS'])
        except (ValueError, KeyError):
            pass
    elif sys.platform == 'darwin':
        try:
            num_cpus = int(os.popen('sysctl -n hw.ncpu').read())
        except ValueError:
            pass
    else:
        try:
            num_cpus = os.sysconf('SC_NPROCESSORS_ONLN')
        except (ValueError, OSError, AttributeError):
            pass
    return num_cpus


def exec_commands(cmds):
    '''
    exec commands in parallel in multiple process
    (as much as we have CPU)

    Attributes:
    -----------
    cmds : list
        a list of commands to be executed

    Returns:
    --------
    None
    '''
    if not cmds:
        return  # empty list

    def done(p):
        return p.poll() is not None

    def success(p):
        return p.returncode == 0

    def fail():
        sys.exit(1)

    max_tasks = cpu_count()
    processes = []
    while True:
        while cmds and len(processes) < max_tasks:
            task = cmds.pop()
            print list2cmdline(task)
            processes.append(Popen(task))

        for p in processes:
            if done(p):
                if success(p):
                    processes.remove(p)
                else:
                    fail()

        if not processes and not cmds:
            break
        else:
            time.sleep(0.05)

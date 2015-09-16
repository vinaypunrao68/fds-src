#!/usr/bin/python
# Copyright 2015 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com

import os
import subprocess
import sys

import config
import lib 

class FDS(object):
    
    def __init__(self):
        pass
    
    def check_status(self):
        '''
        Run the FDS tool with 'status' argument to check if all the
        fds related processes are running.
        
        Returns:
        --------
        bool : if all the processes are running or not.
        '''
        cmd = config.FDS_CMD % (config.FDS_TOOLS, "status")
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
        # skip [WARN]
        proc.stdout.readline()
        # look for interesting data in remaining
        for line in proc.stdout:
            # terminate after processing the [INFO] section
            if line.startswith('[INFO] :  redis'):
                self.status['redis'] = True
                for line in proc.stdout:
                    pass
                break
            # get the process status
            name, remaining = line.split(' ', 1)
            if 'Not running' in remaining:
                return False
            # print "%s starts with '%s'" % (line.strip(), name)
        proc.wait()
        return True

    def stop_single_node(self):
        '''
        Run the FDS tool with 'stop' argument to stop all the fds related
        processes.
        '''
        cmd = config.FDS_CMD % (config.FDS_TOOLS, "stop")
        try:
            subprocess.call([cmd], shell=True)
            if self.check_status() is not False:
                raise Exception("FDS failed to stop")
        except Exception, e:
            lib.log.exception(e)
            sys.exit(1)
    
    def start_single_node(self):
        '''
        Run the FDS tool with 'cleanstart' argument to start all the fds
        related processes.
        '''
        cmd = config.SYSTEM_CMD % (config.SYSTEM_FRAMEWORK,
                                   config.SYSTEM_FRAMEWORK)
        print cmd
        try:
            subprocess.call([cmd], shell=True)
            if not self.check_status():
                raise Exception("FDS multinode cluster failed to start")
        except Exception, e:
            lib.log.exception(e)
            sys.exit(1)

if __name__ == "__main__":
    fds_node = FDS()
    if not fds_node.check_status():
        fds_node.start_single_node()
    else:
        fds_node.stop_single_node()
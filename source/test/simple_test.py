#!/usr/bin/env python

import subprocess
import re
import time
import sys
import argparse
import pdb 

'''
simple_saas.py -- Script to wrap bring_up.py to run simple_up.cfg test in a loop.
'''


def grep_om(log_loc):
    
    om_re = re.compile('OM deployed DLT with \d+ nodes', re.IGNORECASE)
    
    fh = None
    try:
        fh = open(log_loc, 'r')
    except IOError as e:
        print "IOError!", e
        sys.exit(1)
        

    for line in fh:
        match = re.search(om_re, line)
        if match is not None:
            return True

    return False
    

def call_bringup(log_loc):
    
    # Call bring_up.py with first config -- will bring up nodes
    print "Trying to call /fds/sbin/fds-tool.py -f /fds/sbin/intel.cfg -u ..."
        
    proc = subprocess.Popen(['/fds/sbin/fds-tool.py', '-f', '/fds/sbin/intel.cfg', '-u'])
    stdout, stderr = proc.communicate()

    print "Sucess! Moving on to sleep phase..."

    # Wait 10 minutes
    print "Sleeping for 10 minutes..."
    for i in range(10):
        print "Minute", i, "of 10..."
        time.sleep(60)
    
    print "Waking up!"
        
    # Grep om_log
    res = grep_om(log_loc)
    if not res:
        print "Failed to find", grep_for, "in", log_loc
        sys.exit(1)
    else:
        print "All systems are GO!"

        
    # Call the IO script
    out = subprocess.Popen(['/tmp/uploads3.sh'])
    out.wait()


def call_bringdown():
    # Call bring_up.py with first config -- will bring up nodes
    
    out = subprocess.check_output(['/fds/sbin/fds-tool.py', '-f', '/fds/sbin/intel.cfg', '-d', '-c'])
    out.wait()
    

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('-i', type=int, default=100, 
                        help='Number of times to run the bring_up loop.')
    parser.add_argument('--log_location', help='Location of om log',
                        default='/fds/var/logs/om_0.log')

    args = parser.parse_args()

    for i in range(args.i):
        call_bringup(args.log_location)
        call_bringdown()

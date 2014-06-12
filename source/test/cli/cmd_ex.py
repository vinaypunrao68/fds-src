import sys
import os
import logging
sys.path.append('/home/nbayyana/fds-src/source/test')
sys.path.append('/home/nbayyana/fds-src/source/test/fdslib/pyfdsp')

import argh
import cmd
from tabulate import tabulate


import time
from multiprocessing import Process

from fdslib.SvcHandle import *
import struct
import socket
from FDS_ProtocolInterface.ttypes import *

from fdslib import ProcessUtils
from fdslib import IOGen

# Globals
log = None
parser = None
svc_map = SvcMap()

def add_node(id):
    cluster.add_node(id)

def counters(node, svc):
    cntrs = svc_map.client(node, svc).getCounters('*')
    return tabulate([(k,v) for k,v in cntrs.iteritems()],
                    headers=['counter', 'value'])

def list(node, svc, verbose = False):
    return 'list {}:{}:{}'.format(node, svc, verbose)

def setflag(node, svc, id, val):
    try:
        svc_map.client(node, svc).setFlag(id, int(val))
        return 'Success'
    except:
        log.warn(sys.exc_info()[0])
        return 'Unable to set flag: {}'.format(id)

def printflag(node, svc, id=None):
    try:
        if id is None:
            flags = svc_map.client(node, svc).getFlags(None)
            return tabulate([(k,v) for k,v in flags.iteritems()],
                            headers=['flag', 'value'])
        else:
            return svc_map.client(node, svc).getFlag(id)
    except:
        log.warn(sys.exc_info()[0])
        return 'Unable to get flag: {}'.format(id)


class MyShell(cmd.Cmd):
    prompt = 'FDS::>'
    """
    def __init__(self):
        self.parser = argh.ArghParser()
        self.parser.add_commands([add_node, counters, list, setflag, printflag])
        """

    def onecmd(self, line):
        try:
            parser.dispatch(argv=line.split())
        except:
            pass
        return None

if __name__ == '__main__':
    log = ProcessUtils.setup_logger()
    parser = argh.ArghParser()
    parser.add_commands([add_node, counters, list, setflag, printflag])
    MyShell().cmdloop()

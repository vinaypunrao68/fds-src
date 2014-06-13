import sys
import os
import logging
sys.path.append('/home/nbayyana/fds-src/source/test')
sys.path.append('/home/nbayyana/fds-src/source/test/fdslib/pyfdsp')

from argh import *
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

@arg('nodeid', type=long)
def counters(nodeid, svc):
    cntrs = svc_map.client(nodeid, svc).getCounters('*')
    return tabulate([(k,v) for k,v in cntrs.iteritems()],
                    headers=['counter', 'value'])

@arg('nodeid', type=long)
def setflag(nodeid, svc, id, val):
    try:
        svc_map.client(nodeid, svc).setFlag(id, int(val))
        return 'Success'
    except:
        log.warn(sys.exc_info()[0])
        return 'Unable to set flag: {}'.format(id)

@arg('nodeid', type=long)
def printflag(nodeid, svc, id=None):
    try:
        if id is None:
            flags = svc_map.client(nodeid, svc).getFlags(None)
            return tabulate([(k,v) for k,v in flags.iteritems()],
                            headers=['flag', 'value'])
        else:
            return svc_map.client(nodeid, svc).getFlag(id)
    except:
        log.warn(sys.exc_info()[0])
        return 'Unable to get flag: {}'.format(id)

def svclist():
    svc_list = svc_map.list();
    return tabulate(svc_list, headers=['nodeid:svc', 'ip', 'port'])

def refresh():
    svc_map.refresh()

class MyShell(cmd.Cmd):
    prompt = 'FDS::>'
    """
    def __init__(self):
        self.parser = argh.ArghParser()
        self.parser.add_commands([add_node, counters, setflag, printflag])
        """

    def onecmd(self, line):
        try:
            parser.dispatch(argv=line.split())
        except:
            pass
        return None

if __name__ == '__main__':
    log = ProcessUtils.setup_logger()
    parser = ArghParser()
    parser.add_commands([refresh, add_node, counters, svclist, setflag, printflag])
    MyShell().cmdloop()

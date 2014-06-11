import sys
import os
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
        svc_map.client(node, svc).setFlag(id, val)
        return 'Success'
    except:
        return 'Unable to set flag: {}'.format(id)

def getflag(node, svc, id):
    try:
        val = svc_map.client(node, svc).getFlag(id)
        return 'Success'
    except:
        return 'Unable to get flag: {}'.format(id)


parser = argh.ArghParser()
parser.add_commands([add_node, counters, list, setflag])
parser.dispatch()

# assembling:
"""
svc_info = FDSP_Node_Info_Type()
svc_info.control_port = 8903;
svc_info.ip_lo_addr = struct.unpack("!I", socket.inet_aton('127.0.0.1'))[0]
om_svc = OMService(svc_info)

"""


"""
class MyShell(cmd.Cmd):
    def onecmd(self, line):
        parser.dispatch(argv=line.split())
        return None
# dispatching:

if __name__ == '__main__':
    ProcessUtils.setup_logger()

    (options, args) = ProcessUtils.parse_fdscluster_args()

    cluster = FdsCluster(options.config_file)
    time.sleep(5)

    MyShell().cmdloop()
    """

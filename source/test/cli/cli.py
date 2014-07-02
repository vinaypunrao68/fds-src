import sys
import os
import logging
sys.path.append('../../test')
sys.path.append('../fdslib/pyfdsp')

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

"""
Cli exit exception
"""
class CliExitException(Exception):
    pass

# Globals
log = None
parser = None
svc_map = None

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
    except Exception, e:
        log.exception(e)
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
    except Exception, e:
        log.exception(e)
        return 'Unable to get flag: {}'.format(id)

def svclist():
    svc_list = svc_map.list();
    return tabulate(svc_list, headers=['nodeid:svc', 'ip', 'port'])

def refresh():
    svc_map.refresh()

def exit():
    # We could do sys.exit here.  But any invalid command also raises
    # sys.exit and we ignoring those for now.  To cleanly exit we use
    # CliExitException
    raise CliExitException()

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
        except CliExitException:
            print 'Goodbye!'
            sys.exit(0)
        except Exception, e:
            log.warn('cmd: {} exception: {}'.format(line, sys.exc_info()[0]))
            log.exception(e)
            pass
        except:
            pass
        return None

def main(ip='127.0.0.1', port=7020, command_line=None):
    """
    Main entry point. Creates the argument parser, shell, and svc map objects
    ip - ip of service that provides the svc map
    port - port of service that provides the svc map
    """
    global parser
    global svc_map
    parser = ArghParser()
    parser.add_commands([refresh,
                         add_node,
                         counters,
                         svclist,
                         setflag,
                         printflag,
                         exit])
    svc_map = SvcMap(ip, port)
    if command_line is not None:
	MyShell().onecmd(command_line)
    else:
        MyShell().cmdloop()

if __name__ == '__main__':
    log = ProcessUtils.setup_logger(file = 'console.log')
    # Just for parsing arguments for main function
    argv = sys.argv
    argv[0] = 'main'
    temp_parser = ArghParser()
    temp_parser.add_commands([main])
    temp_parser.dispatch(argv)

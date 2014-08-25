import sys
import os
import logging
sys.path.append('../../test')
sys.path.append('../fdslib/pyfdsp')
sys.path.append('../fdslib')

from argh import *
import cmd
from tabulate import tabulate


import time
from multiprocessing import Process
import json

from SvcHandle import *
import struct
import socket
from FDS_ProtocolInterface.ttypes import *

from fdslib import ProcessUtils
from fdslib import IOGen
from fdslib import thrift_json

"""
Cli exit exception
"""
class CliExitException(Exception):
    pass

# Globals
log = None
parser = None
svc_map = None
src_id_counter = 0

@arg('policy-name', help= "-snapshot policy name ")
@arg('snap-frequency', help= "-frequency of the snapshot")
@arg('retension-time', help= "-retension time for the snapshot")
@arg('timeof-day', help= "-time for the  creating the snapshot")
@arg('dayof-month', help= "-day for creating the snapshot")
def create_snap_policy(policy_name, snap_frequency, retension_time, timeof_day, dayof_month):
    try:
        #svc_map.client(nodeid, svc).fdsp_expireSnap(val)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'snapshot policy failed: {}'.format(policy_name)

@arg('vol-name', help= "-volume name of the snapshot")
@arg('policy-name', help= "-policy name for the snapshot")
def create_snap(vol_name, policy_name):
    try:
        #svc_map.client(nodeid, svc).fdsp_expireSnap(val)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'create snapshot failed: {}'.format(vol_name)

@arg('snap-name', help= "-snap shot name for deleting")
def delete_snap(snap_name):
    try:
        #svc_map.client(nodeid, svc).fdsp_expireSnap(val)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'delete snapshot failed: {}'.format(snap_name)

@arg('vol-name', help= "-Volume name for attaching snap policy")
@arg('policy-name', help= "-snap shot policy name")
def attach_snap_policy(vol_name, policy_name):
    try:
        #svc_map.client(nodeid, svc).fdsp_expireSnap(val)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'attach snap policy  failed: {}'.format(vol_name)

@arg('vol-name', help= "-Volume name for detaching snap policy")
@arg('policy-name', help= "-snap shot policy name")
def detach_snap_policy(vol_name, policy_name):
    try:
        #svc_map.client(nodeid, svc).fdsp_expireSnap(val)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'detach snap policy failed: {}'.format(vol_name)

@arg('vol-name', help= "-Volume name  of the clone")
@arg('clone-name', help= "-name of  the  volume clone")
@arg('policy-name', help= "-clone policy name")
def create_clone(vol_name, clone_name, policy_name):
    try:
        #svc_map.client(nodeid, svc).fdsp_expireSnap(val)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'create clone failed: {}'.format(vol_name)

@arg('vol-name', help= "-volume name  of the clone")
@arg('clone-name', help= "-name of  the  volume clone for delete")
def delete_clone(vol_name, clone_name):
    try:
        #svc_map.client(nodeid, svc).fdsp_expireSnap(val)
        return 'Success'
    except Exception, e:
        return 'delete clone  failed: {}'.format(vol_name)

@arg('vol-name', help= "-volume name  of the clone")
@arg('clone-name', help= "-name of  the  volume clone for restore")
def restore_clone(vol_name, clone_name):
    try:
        #svc_map.client(nodeid, svc).fdsp_expireSnap(val)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'restore clone failed: {}'.format(vol_name)

@arg('vol-name', help= "-volume name  for listing the snap")
def list_snap(vol_name):
    try:
        #svc_map.client(nodeid, svc).fdsp_expireSnap(val)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'list snap  failed: {}'.format(vol_name)

def add_node(id):
    cluster.add_node(id)

@arg('nodeid', type=long)
def counters(nodeid, svc):
    cntrs = svc_map.client(nodeid, svc).getCounters('*')
    return tabulate([(k,v) for k,v in cntrs.iteritems()],
                    headers=['counter', 'value'])

@arg('nodeid', type=long)
def reset_counters(nodeid, svc):
    cntrs = svc_map.client(nodeid, svc).resetCounters('*')
    print "Reset all counters"
    #return tabulate([(k,v) for k,v in cntrs.iteritems()],
    #                headers=['counter', 'value'])

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

def dumpjson(th_type, file_name):
    thismodule = sys.modules[__name__]
    d = thrift_json.th_obj_to_dict(getattr(thismodule, th_type))
    with open(file_name, 'w') as json_file:
        json.dump(d, json_file)
    return 'Success'

@arg('nodeid', type=long)
def sendjson(nodeid, svc, th_type, file_name):
    global src_id_counter
    with open(file_name, 'r') as json_file:
        thismodule = sys.modules[__name__]
        th_obj = thrift_json.dict_to_th_obj(getattr(thismodule, th_type),
                                            json.load(json_file))
        th_obj.validate()
        transportOut = TTransport.TMemoryBuffer()
        protocolOut = TBinaryProtocol.TBinaryProtocol(transportOut)
        th_obj.write(protocolOut)

        # AsyncHdr setup and actual sending of object
        th_type_id = th_type + 'TypeId'
        header = AsyncHdr(
            msg_chksum      =   123456,  # dummy file
            msg_code        =   123456, # more dummy
            msg_type_id     =   FDSPMsgTypeId._NAMES_TO_VALUES[th_type_id],
            msg_src_id      =   src_id_counter,
            msg_src_uuid    =   SvcUuid(int('cac0', 16)),
            msg_dst_uuid    =   SvcUuid(nodeid)  # later implement actual serviceid
            )

        client = svc_map.client(nodeid, svc)
        client.asyncReqt(header, transportOut.getvalue())
        src_id_counter += 1
    return 'Succesfully sent json'

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
        self.parser.add_commands([add_node, counters, reset_counters, setflag, printflag])
        """

    def onecmd(self, line):
        try:
            parser.dispatch(argv=line.split())
        except CliExitException:
            print 'Goodbye!'
            sys.exit(0)
        except Exception, e:
            print 'Failed'
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
    parser.add_commands([create_snap_policy,
                         create_snap,
                         delete_snap,
                         attach_snap_policy,
                         detach_snap_policy,
                         create_clone,
                         delete_clone,
                         restore_clone,
                         list_snap,
                         refresh,
                         add_node,
                         counters,
                         reset_counters,
                         svclist,
                         setflag,
                         printflag,
                         dumpjson,
                         sendjson,
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

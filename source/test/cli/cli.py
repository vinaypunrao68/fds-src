#!/usr/bin/env python

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
from snapshot.ttypes import *

from fdslib import process
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
unused = 0

@arg('policy-name', help= "-snapshot policy name ")
@arg('recurrence-rule', help= "- ical Rule format: FREQ=MONTHLY;BYDAY=FR;BYHOUR=23;BYMINUTE=30;BYSECOND=0;COUNT=5 ")
@arg('retension-time', type=long, help= "-retension time for the snapshot")
def create_snap_policy(policy_name, recurrence_rule, retension_time):
    try:
        snap_policy = SnapshotPolicy(
            id                    =   0,
            policyName            =   policy_name,
            recurrenceRule        =   recurrence_rule,
            retentionTimeSeconds  =   retension_time
            )
        # get the  OM client  handler
        client = svc_map.omConfig()
        # invoke the thrift  interface call
        policy_id = client.createSnapshotPolicy(snap_policy)
        return  ' Successfully created  snapshot policy: {}'.format(policy_id)
    except Exception, e:
        log.exception(e)
        return ' creating snapshot policy failed: {}'.format(policy_name)

@arg('policy-id', type=long, help= "-snapshot policy Identifier")
def delete_snap_policy(policy_id):
    try:
        # get the  OM client  handler
        client = svc_map.omConfig()
        # invoke the thrift  interface call
        client.deleteSnapshotPolicy(policy_id)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return ' delete  snapshot policy failed: {}'.format(policy_id)
def list_snap_policy():
    try:
        # get the  OM client  handler
        client = svc_map.omConfig()
        # invoke the thrift  interface call
        policy_list = client.listSnapshotPolicies(unused)
	return tabulate([(item.policyName, item.recurrenceRule, item.id,  item.retentionTimeSeconds) for item in policy_list],
			headers=['Policy-Name', 'Policy-Rule', 'Policy-Id', 'Retension-Time'])
    except Exception, e:
        log.exception(e)
        return ' delete  snapshot policy failed: {}'.format(policy_id)

@arg('vol-name', help= "-Volume name for attaching snap policy")
@arg('policy-id', type=long,  help= "-snap shot policy id")
def attach_snap_policy(vol_name, policy_id):
    try:
        # get the  OM client  handler
        client = svc_map.omConfig()
        # invoke the thrift  interface call
        volume_id  = client.getVolumeId(vol_name);
        client.attachSnapshotPolicy(volume_id, policy_id)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'attach snap policy  failed: {}'.format(vol_name)

@arg('vol-name', help= "-Volume name for detaching snap policy")
@arg('policy-id', type=long,  help= "-snap shot policy name")
def detach_snap_policy(vol_name, policy_id):
    try:
        # get the  OM client  handler
        client = svc_map.omConfig()
        # invoke the thrift  interface call
        volume_id  = client.getVolumeId(vol_name);
        client.detachSnapshotPolicy(volume_id, policy_id)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'detach snap policy failed: {}'.format(vol_name)

@arg('policy-id', type=long, help= "-snap shot policy id")
def list_volume_snap_policy(policy_id):
    try:
        # get the  OM client  handler
        client = svc_map.omConfig()
        # invoke the thrift  interface call
        volume_list =  client.listVolumesForSnapshotPolicy(policy_id)
	return tabulate([(item) for item in volume_list],
			headers=['Volume-Id'])
    except Exception, e:
        log.exception(e)
        return ' list Volumes for snapshot policy failed: {}'.format(policy_id)

@arg('vol-name', help= "-Volume name for listing snap policy")
def list_snap_policy_volume(vol_name):
    try:
        # get the  OM client  handler
        client = svc_map.omConfig()
        # invoke the thrift  interface call
        volume_id  = client.getVolumeId(vol_name);
        policy_list = client.listSnapshotPoliciesForVolume(volume_id)
	return tabulate([(item.policyName, item.recurrenceRule, item.id,  item.retentionTimeSeconds) for item in policy_list],
			headers=['Policy-Name', 'Policy-Rule', 'Policy-Id', 'Retension-Time'])
    except Exception, e:
        log.exception(e)
        return ' list snapshot policies for volumes failed: {}'.format(vol_name)

@arg('vol-name', help= "-list snapshot for Volume name")
def list_snapshot(vol_name):
    try:
        # get the  OM client  handler
        client = svc_map.omConfig()
        # invoke the thrift  interface call
        volume_id  = client.getVolumeId(vol_name);
        snapshot = client.listSnapshots(volume_id)
	return tabulate([(item.snapshotName, item.volumeId, item.snapshotId, item.snapshotPolicyId, time.ctime((item.creationTimestamp)/1000)) for item in snapshot],
			headers=['Snapshot-name', 'volume-Id', 'Snapshot-Id', 'policy-Id', 'Creation-Time'])
    except Exception, e:
        log.exception(e)
        return ' list snapshot polcies  snapshot polices for volume failed: {}'.format(vol_name)

@arg('vol-name', help= "-Volume name  of the clone")
@arg('clone-name', help= "-name of  the  volume clone")
@arg('policy-name', help= "-clone policy name")
def create_clone(vol_name, clone_name, policy_name):
    try:
        # get the  OM client  handler
        client = svc_map.omConfig()
        # invoke the thrift  interface call
        volume_id  = client.getVolumeId(vol_name);
        client.cloneVolume(volume_id, policy_name, clone_name )
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'create clone failed: {}'.format(vol_name)

@arg('vol-name', help= "-volume name  of the clone")
@arg('clone-name', help= "-name of  the  volume clone for restore")
def restore_clone(vol_name, clone_name):
    try:
        # get the  OM client  handler
        client = svc_map.omConfig()
        # invoke the thrift  interface call
        volume_id  = client.getVolumeId(vol_name);
        client.restoreClone(volume_id, snapshotName)
        return 'Success'
    except Exception, e:
        log.exception(e)
        return 'restore clone failed: {}'.format(vol_name)

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
    prompt = 'fds:> '
    """
    def __init__(self):
        self.parser = argh.ArghParser()
        self.parser.add_commands([add_node, counters, reset_counters, setflag, printflag])
        """

    def onecmd(self, line):
        try:
            parser.dispatch(argv=line.split())
        except CliExitException:
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
                         delete_snap_policy,
                         list_snap_policy,
                         attach_snap_policy,
                         detach_snap_policy,
                         list_volume_snap_policy,
                         list_snap_policy_volume,
                         list_snapshot,
                         create_clone,
                         restore_clone,
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
        try:
            MyShell().cmdloop()
        except KeyboardInterrupt:
            print ''
            sys.exit(0)

if __name__ == '__main__':
    log = process.setup_logger(file = 'console.log')
    # Just for parsing arguments for main function
    argv = sys.argv
    argv[0] = 'main'
    temp_parser = ArghParser()
    temp_parser.add_commands([main])
    temp_parser.dispatch(argv)

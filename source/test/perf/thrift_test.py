#!/usr/bin/python
import os, sys, re, random, hashlib

fds_home = "/home/monchier/FDS/"

sys.path.append(os.path.join(fds_home, 'source/test/fdslib'))
sys.path.append(os.path.join(fds_home, 'source/test/fdslib/pyfdsp'))
sys.path.append(os.path.join(fds_home, 'source/test/fdslib/pyfdsp'))
sys.path.append(os.path.join(fds_home, 'source/test/fdslib/pyfdsp/fds_service'))
print sys.path
from SvcHandle import *
from BaseAsyncSvc import *

src_id_counter = 0

def sendjson_put(nodeid, svc, svc_map, th_type):
    global src_id_counter
    myobject = os.urandom(4096)
    hash = hashlib.sha1(myobject)
    #hash.update(myobject)
    myhash = hash.digest()
    FDS_ObjectIdType
    print "hash", myhash.encode('hex')
    th_obj = PutObjectMsg(
        volume_id = svc_map.omConfig().getVolumeId("volume0"),
        origin_timestamp = 1,
        data_obj_id = FDS_ObjectIdType(digest=myhash),
        data_obj_len = 4096,
        data_obj = myobject
    )
    # print th_obj
    
    # populate th_obj
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
    return 'Succesfully sent json', myhash

def sendjson_get(nodeid, svc, svc_map, th_type, myhash):
    global src_id_counter
    volid = svc_map.omConfig().getVolumeId("volume0")
    obj_id = FDS_ObjectIdType(digest=myhash)
    print volid, obj_id
    th_obj = GetObjectMsg(
        volume_id = volid,
        data_obj_id = obj_id,
    )
    # print th_obj

    # populate th_obj
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

def main():
    ip = "10.1.10.139"
    port = 7020
    svc_map = SvcMap(ip, port)
    svclist = svc_map.list()
    print svclist
    for e in svclist:
        nodeid, svc = e[0].split(":")
        nodeid = long(nodeid)
        ip = e[1]
        if svc == "sm" and re.match("10\.1\.10\.\d+",ip):
            #cntrs = svc_map.client(nodeid, svc).getCounters('*')
            # header = asyncHdr()
            # print header
            msg, h = sendjson_put(nodeid, svc, svc_map, "PutObjectMsg")
            print msg
            time.sleep(5)
            #cntrs = svc_map.client(nodeid, svc).getCounters('*')
            #print cntrs
            time.sleep(5)
            print sendjson_get(nodeid, svc, svc_map, "GetObjectMsg", h)
            time.sleep(5)
            #cntrs = svc_map.client(nodeid, svc).getCounters('*')
            #print cntrs


if __name__ == "__main__":
    main()

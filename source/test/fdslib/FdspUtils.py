#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import logging
import sys
import time
import hashlib
import types
from FDS_ProtocolInterface.ttypes import *

from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from common.ttypes import *
from common.constants import *
from svc_api.ttypes import *
from svc_types.ttypes import *
from svc_types.constants import *
from dm_api.ttypes import *
from dm_api.constants import *
import sm_api as smapi
import sm_types as smtypes


log = logging.getLogger(__name__)

##
# @brief Returns message object in fds_service object based on the typeId
#
# @param typeId is an interger (see FDSPMsgTypeId)
#
# @return Service message object 
def newSvcMsgByTypeId(typeId):
    if type(typeId) == types.StringType:
        typeStr=typeId
    else:
        typeStr = FDSPMsgTypeId._VALUES_TO_NAMES[typeId]
    thType = typeStr.replace('TypeId','')
    log.info('th typestr: {}, typeid: {}'.format(typeStr, thType))
    thismodule = sys.modules[__name__]
    thObj = None
    if hasattr(thismodule, thType):
        thObj = getattr(thismodule, thType)()
    else:
        thType = thType.replace('RspMsg','MsgRsp')
        thObj = getattr(thismodule, thType)()
    if thObj is None:
        log.error('unable to find thrift object : {}'.format(thType))
    return thObj

def getMsgTypeId(msg):
    thTypeName = '{}TypeId'.format(msg.__class__.__name__)
    return FDSPMsgTypeId._NAMES_TO_VALUES[thTypeName]

##
# @brief Deserialize payload to service message from fds_service module
#
# @param asyncHdr
# @param payload
#
# @return 
def deserializeSvcMsg(asyncHdr, payload):
    svcMsg = newSvcMsgByTypeId(asyncHdr.msg_type_id)
    transportIn = TTransport.TMemoryBuffer(payload)
    protocolIn = TBinaryProtocol.TBinaryProtocol(transportIn)
    svcMsg.read(protocolIn)
    return svcMsg

##
# @brief Serializes service message object into a string
#
# @param msg thrift service message object
#
# @return serialized string
def serializeSvcMsg(msg):
    transportOut = TTransport.TMemoryBuffer()
    protocolOut = TBinaryProtocol.TBinaryProtocol(transportOut)
    msg.write(protocolOut)
    return transportOut.getvalue()

##
# @brief computes sha1 of data and returns digest embedded in FDS_ObjectIdType
#
# @param data as string
#
# @return 
def genFdspObjectId(data):
    m = hashlib.sha1()
    m.update(data)
    return FDS_ObjectIdType(digest=m.digest())

#--------------------------------------------------------------------------
# Message generation routines
#--------------------------------------------------------------------------

##
# @brief Returns async header object
#
# @param mySvcUuid int service uuid
# @param targetSvcUuid int service uuid
# @param reqId
# @param msg thrift service message object
#
# @return 
def newAsyncHeader(mySvcUuid, targetSvcUuid, reqId, msg):
    header = AsyncHdr(
        msg_chksum      =   123456,  # dummy file
        msg_code        =   0, # more dummy
        msg_type_id     =   getMsgTypeId(msg),
        msg_src_id      =   reqId,
        msg_src_uuid    =   SvcUuid(mySvcUuid),
        msg_dst_uuid    =   SvcUuid(targetSvcUuid)  # later implement actual serviceid
    )
    return header

##
# @brief 
#
# @param svcUuid - integral svc uuid
# @param ip - string ip
# @param port - port
# @param svcType - FDSP.FDSP_MgrIdType type
# @return 
def newUuidBindMsg(svcUuid, ip, port, svcType):
    svcId = SvcID(svc_uuid=SvcUuid(svcUuid), svc_name='client')
    msg = UuidBindMsg(svc_id=svcId, 
                      svc_addr=ip,
                      svc_port=port,
                      svc_node=svcId,
                      svc_auto_name='default',
                      svc_type=svcType) 
    return msg

##
# @brief 
#
# @param svcUuid - integral svc uuid
# @param ip - string ip
# @param port - port
# @param svcType - FDSP.FDSP_MgrIdType type
# @return 
def newNodeInfoMsg(svcUuid, ip, port, svcType):
    msg = NodeInfoMsg()
    msg.node_loc = newUuidBindMsg(svcUuid=svcUuid,
                                  ip=ip,
                                  port=port,
                                  svcType=svcType)
    msg.node_domain = DomainID(domain_id=SvcUuid(0), domain_name="default")
    msg.node_stor = None
    msg.nd_base_port = port
    msg.nd_svc_mask = 1 # (TODO: Rao) ask vy
    msg.nd_bcast = False
    return msg

##
# @brief 
#
# @param volId volume id as long
# @param data data as string(binary should work)
#
# @return 
def newPutObjectMsg(volId, data):
    msg = PutObjectMsg()
    msg.volume_id = volId
    msg.origin_timestamp = int(time.time())
    msg.data_obj_id = genFdspObjectId(data)
    msg.data_obj_len = len(data)
    msg.data_obj = data
    return msg

##
# @brief 
#
# @param volId as long
# @param objectId as string
#
# @return 
def newGetObjectMsg(volId, objectId):
    msg = GetObjectMsg()
    msg.volume_id = volId
    msg.data_obj_id = FDS_ObjectIdType(digest=objectId)
    return msg

##
# @brief 
#
# @param volId as long
#
# @return 
def newGetVolumeMetaDataMsg(volId):
    msg = GetVolumeMetadataMsg()
    msg.volumeId = volId
    return msg

##
# @brief 
#
# @param volId as long
#
# @return 

def newGetBucketMsg(volId, startOff, maxKey):
    msg = GetBucketMsg()
    msg.volume_id = volId
    msg.startPos = startOff
    msg.count = maxKey
    return msg

##
# @brief 
#
# @param none
#
# @return 

def newGetDmStatsMsg(volId):
    msg = GetDmStatsMsg()
    msg.volume_id = volId
    return msg

def newEnableScavengerMsg():
    msg = smapi.ttypes.CtrlNotifyScavenger()
    msg.scavenger = smtypes.ttypes.FDSP_ScavengerType()
    msg.scavenger.cmd = smtypes.ttypes.FDSP_ScavengerCmd.FDSP_SCAVENGER_ENABLE
    return msg

def newDisableScavengerMsg():
    msg = smapi.ttypes.CtrlNotifyScavenger()
    msg.scavenger = smtypes.ttypes.FDSP_ScavengerType()
    msg.scavenger.cmd = smtypes.ttypes.FDSP_ScavengerCmd.FDSP_SCAVENGER_DISABLE
    return msg


def newStartScavengerMsg():
    msg = smapi.ttypes.CtrlNotifyScavenger()
    msg.scavenger = smtypes.ttypes.FDSP_ScavengerType()
    msg.scavenger.cmd = smtypes.ttypes.FDSP_ScavengerCmd.FDSP_SCAVENGER_START
    return msg

def newStopScavengerMsg():
    msg = smapi.ttypes.CtrlNotifyScavenger()
    msg.scavenger = smtypes.ttypes.FDSP_ScavengerType()
    msg.scavenger.cmd = smtypes.ttypes.FDSP_ScavengerCmd.FDSP_SCAVENGER_STOP
    return msg

def newScavengerStatusMsg():
    msg = CtrlQueryScavengerStatus()
    return msg

def newScavengerProgressMsg():
    msg = CtrlQueryScavengerProgress()
    return msg

def newSetScavengerPolicyMsg(t1, t2, t3, t4):
    msg = CtrlSetScavengerPolicy()
    msg.dsk_threshold1 = t1
    msg.dsk_threshold2 = t2
    msg.token_reclaim_threshold = t3
    msg.tokens_per_dsk = t4
 
    return msg

def newQueryScavengerPolicyMsg():
    msg = smapi.ttypes.CtrlQueryScavengerPolicy()
    return msg

def newQueryPolicyMsg():
    msg = smapi.ttypes.CtrlQueryTierPolicy()
    return msg

def newQueryScrubberStatusMsg():
    msg = smapi.ttypes.CtrlQueryScrubberStatus()
    return msg

def newEnableScrubberMsg():
    msg = smapi.ttypes.CtrlSetScrubberStatus()
    msg.scrubber_status = smtypes.ttypes.FDSP_ScrubberStatusType.FDSP_SCRUB_ENABLE
    return msg

def newDisableScrubberMsg():
    msg = smapi.ttypes.CtrlSetScrubberStatus()
    msg.scrubber_status = smtypes.ttypes.FDSP_ScrubberStatusType.FDSP_SCRUB_DISABLE
    return msg

def newShutdownMODMsg():
    msg = ShutdownMODMsg()
    return msg

def newCtrlStartHybridTierCtrlrMsg():
    msg = CtrlStartHybridTierCtrlrMsg()
    return msg

def newStartSmchkMsg(targetTokens):
    msg = smapi.ttypes.CtrlNotifySMCheck()
    msg.SmCheckCmd = smtypes.ttypes.SMCheckCmd.SMCHECK_START
    if targetTokens is not None:
        msg.targetTokens = set(targetTokens)

    return msg

def newStopSmchkMsg():
    msg = smapi.ttypes.CtrlNotifySMCheck()
    msg.SmCheckCmd = smtypes.ttypes.SMCheckCmd.SMCHECK_STOP

    return msg

def newDmchkMsg(volId, source, remedy):
    msg = smapi.ttypes.CtrlNotifyDMStartMigrationMsg()
    msg.migrations.VolDescriptors.volUUID = volId
    msg.migrations.source = source
    msg.dryRun = remedy

    return msg

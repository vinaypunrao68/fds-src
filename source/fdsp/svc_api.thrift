/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "svc_types.thrift"

include "FDSP.thrift"
include "common.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.svc

/* ------------------------------------------------------------
   Operations on Services
   ------------------------------------------------------------*/

/**
 * Notify of update to DLT.
 */
struct CtrlNotifyDLTUpdate {
  1: svc_types.FDSP_DLT_Data_Type   dlt_data;
  2: i64                            dlt_version;
}

/**
 * Notify addition of volume.
 */
struct CtrlNotifyVolAdd {
  /** Volume settings and attributes. */
  1: common.FDSP_VolumeDescType vol_desc;
  /**  */
  2: FDSP.FDSP_NotifyVolFlag    vol_flag;
}

/**
 * Notify removal of volume.
 */
struct CtrlNotifyVolRemove {
  /** Volume settings and attributes. */
  1: common.FDSP_VolumeDescType vol_desc;
  2: FDSP.FDSP_NotifyVolFlag    vol_flag;
}

/**
 * Notify modification to volume.
 */
struct CtrlNotifyVolMod {
  /** Volume settings and attributes. */
  1: common.FDSP_VolumeDescType vol_desc;
  2: FDSP.FDSP_NotifyVolFlag    vol_flag;
}

/**
 * Notify snapshot of volume.
 */
struct CtrlNotifySnapVol {
  /** Volume settings and attributes. */
  1: common.FDSP_VolumeDescType vol_desc;
  2: FDSP.FDSP_NotifyVolFlag    vol_flag;
}

/**
 * Debug message for hybrid tiering
 */
struct CtrlStartHybridTierCtrlrMsg
{
}

/**
 * Empty msg (No-op)
 */
struct EmptyMsg {
}

/**
 * Shutdown service.
 */
struct ShutdownMODMsg {
}

/* ------------------------------------------------------------
   Common Services
   ------------------------------------------------------------*/

service BaseAsyncSvc {
  oneway void asyncReqt(1: svc_types.AsyncHdr asyncHdr, 2: string payload);
  oneway void asyncResp(1: svc_types.AsyncHdr asyncHdr, 2: string payload);
  svc_types.AsyncHdr uuidBind(1: svc_types.UuidBindMsg msg);
}

service PlatNetSvc extends BaseAsyncSvc {
    oneway void allUuidBinding(1: svc_types.UuidBindMsg mine);
    oneway void notifyNodeActive(1: FDSP.FDSP_ActivateNodeType info);

    list<svc_types.NodeInfoMsg> notifyNodeInfo(1: svc_types.NodeInfoMsg info, 2: bool bcast);
    svc_types.DomainNodes getDomainNodes(1: svc_types.DomainNodes dom);
    svc_types.NodeEvent getSvcEvent(1: svc_types.NodeEvent input);

    svc_types.ServiceStatus getStatus(1: i32 nullarg);
    map<string, i64> getCounters(1: string id);
    void resetCounters(1: string id);
    void setConfigVal(1:string id, 2:i64 value);
    void setFlag(1:string id, 2:i64 value);
    i64 getFlag(1:string id);
    map<string, i64> getFlags(1: i32 nullarg);
    /* For setting fault injection.
     * @param cmdline format based on libfiu
     */
    bool setFault(1: string cmdline);
}

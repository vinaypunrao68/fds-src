/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "svc_types.thrift"

include "FDSP.thrift"
include "common.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol

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

/**
 * Bind service to Uuid.
 */
struct UuidBindMsg {
  1: required common.SvcID          svc_id;
  2: required string                svc_addr;
  3: required i32                   svc_port;
  4: required common.SvcID          svc_node;
  5: required string                svc_auto_name;
  6: required FDSP.FDSP_MgrIdType   svc_type;
}

/* ------------------------------------------------------------
   Common Services
   ------------------------------------------------------------*/

service BaseAsyncSvc {
  oneway void asyncReqt(1: svc_types.AsyncHdr asyncHdr, 2: string payload);
  oneway void asyncResp(1: svc_types.AsyncHdr asyncHdr, 2: string payload);
  svc_types.AsyncHdr uuidBind(1: UuidBindMsg msg);
}

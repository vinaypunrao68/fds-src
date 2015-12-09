/*
 * Copyright 2014 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "om_types.thrift"

include "common.thrift"
include "svc_types.thrift"
include "svc_api.thrift"
include "health_monitoring_api.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om

/* ------------------------------------------------------------
   Operations on Volumes
   ------------------------------------------------------------*/

/**
 * Request a Volume's Descriptor
 */
struct GetVolumeDescriptor {
  /** Volume name */
  1: string volume_name;
}

struct GetVolumeDescriptorResp {
  1: svc_types.FDSP_VolumeDescType vol_desc;
}

/* ------------------------------------------------------------
   Operations on Replication
   ------------------------------------------------------------*/

/**
 * Abort any Replication
 */
struct CtrlTokenMigrationAbort {
}

/* ------------------------------------------------------------
   Operations on Node Events
   ------------------------------------------------------------*/

/**
 *
 */
struct CtrlSvcEvent {
  /** Uuid of service which caused error */
  1: required svc_types.SvcUuid    evt_src_svc_uuid;
  /** Error Code */
  2: required i32               evt_code;
  /** Message causing the Error */
  3: svc_types.FDSPMsgTypeId  evt_msg_type_id;
}

// Service action responses from PM
// Keeping it generic, though we use it currently only for start
struct SvcStateChangeResp {
  /* List of services that have received change requests */
  1: om_types.SvcChangeInfoList changeList;
  /* Id of the PM that handled the request */
  2: svc_types.SvcUuid          pmSvcUuid;
}

// Make NotifyHealthReport defined within the fpi namespace.
typedef health_monitoring_api.NotifyHealthReport NotifyHealthReport
typedef health_monitoring_api.HeartbeatMessage HeartbeatMessage
 # OM Service.  Only put sync rpc calls in here.  Async RPC calls use
 # message passing provided by BaseAsyncSvc
service OMSvc extends svc_api.PlatNetSvc {
  /**
  * @brief Use this rpc for registration
  *
  * @param svcInfo
  *
  * @throws If registration fails appropriate excpetion is thrown
  */
  void registerService(1: svc_types.SvcInfo svcInfo) throws (1: om_types.OmRegisterException e);

  /**
  * @brief Returns service infomation identified by svcUuid
  *
  * @param svcUuid
  *
  * @return
  */
  svc_types.SvcInfo getSvcInfo(1: svc_types.SvcUuid svcUuid) throws (1: om_types.SvcLookupException e);

  /**
  * @brief Called by other managers to pull the DMT
  *
  * @param NULL
  */
  svc_api.CtrlNotifyDMTUpdate getDMT(1: i64 nullarg) throws (1: common.ApiException e);

  /**
  * @brief Called by other managers to pull the DLT
  *
  * @param NULL
  */
  svc_api.CtrlNotifyDLTUpdate getDLT(1: i64 nullarg) throws (1: common.ApiException e);

  /**
   * @brief Called to get the volume descriptors
   * @param NULL
   */
   svc_api.GetAllVolumeDescriptors getAllVolumeDescriptors(1: i64 nullarg) throws (1: common.ApiException e);

  /**
   * @brief Supply service endpoint information to remote local domain
   *
   * @param svctype Filter result according to the given service type
   */
   svc_api.GetSvcEndpoints getSvcEndpoints(1: svc_types.FDSP_MgrIdType svctype) throws (1: common.ApiException e);
}

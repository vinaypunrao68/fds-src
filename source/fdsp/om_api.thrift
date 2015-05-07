/*
 * Copyright 2014 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "dm_types.thrift"
include "om_types.thrift"

include "dm_api.thrift"
include "common.thrift"
include "svc_types.thrift"
include "svc_api.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om

/* ------------------------------------------------------------
   Operations on Volumes
   ------------------------------------------------------------*/

/**
 * Test for Volume
 */
struct CtrlTestBucket {
  /** Test Volume Specification */
  1: om_types.FDSP_TestBucket   tbmsg;
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
  1: required common.SvcUuid    evt_src_svc_uuid;
  /** Error Code */
  2: required i32               evt_code;
  /** Message causing the Error */
  3: svc_types.FDSPMsgTypeId  evt_msg_type_id;
}


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
  svc_types.SvcInfo getSvcInfo(1: common.SvcUuid svcUuid) throws (1: om_types.SvcLookupException e);

  /**
  * @brief Called by other managers to pull the DMT
  *
  * @param NULL
  */
  dm_api.CtrlNotifyDMTUpdate getDMT(1: i64 nullarg);

  /**
  * @brief Called by other managers to pull the DLT
  *
  * @param NULL
  */
  svc_api.CtrlNotifyDLTUpdate getDLT(1: i64 nullarg);
}

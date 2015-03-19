/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "om_types.thrift"

include "common.thrift"
include "svc_types.thrift"
include "svc_api.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om

/* ------------------------------------------------------------
   Operations on Volumes
   ------------------------------------------------------------*/

/**
 * Create Volume
 */
struct CtrlCreateBucket {
  /** Create Volume Specification */
  1: common.FDSP_CreateVolType  cv;
}

/**
 * Delete Volume
 */
struct CtrlDeleteBucket {
  /** Delete Volume Specification */
  1:  common.FDSP_DeleteVolType dv;
}

/**
 * Modify Volume
 */
struct CtrlModifyBucket {
  /** Modify Volume Specification */
  1:  common.FDSP_ModifyVolType mv;
}

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
  * @brief Returns service map
  *
  * @param nullarg
  *
  * @return
  */
  list<svc_types.SvcInfo> getSvcMap(1: i64 nullarg);

  /**
  * @brief Returns service infomation identified by svcUuid
  *
  * @param svcUuid
  *
  * @return
  */
  svc_types.SvcInfo getSvcInfo(1: common.SvcUuid svcUuid) throws (1: om_types.SvcLookupException e);
}

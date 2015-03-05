/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "om_types.thrift"

include "common.thrift"
include "fds_service.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om

/* ------------------------------------------------------------
   Operations on Volumes
   ------------------------------------------------------------*/

/**
 *
 */
struct CtrlCreateBucket {
  /**  */
  1: common.FDSP_CreateVolType  cv;
}

/**
 *
 */
struct CtrlDeleteBucket {
  /**  */
  1:  common.FDSP_DeleteVolType dv;
}

/**
 *
 */
struct CtrlModifyBucket {
  /**  */
  1:  common.FDSP_ModifyVolType mv;
}

/**
 *
 */
struct CtrlTestBucket {
  /**  */
  1: om_types.FDSP_TestBucket   tbmsg;
}

/* ------------------------------------------------------------
   Operations on Replication
   ------------------------------------------------------------*/

/**
 *
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
  /**  */
  1: required common.SvcUuid    evt_src_svc_uuid; // The svc uuid that this event targets
  /**  */
  2: required i32               evt_code;         // The error itself
  /**  */
  3: fds_service.FDSPMsgTypeId  evt_msg_type_id;  // The msg that trigged this event (if any)
}

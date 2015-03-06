/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "om_types.thrift"

include "common.thrift"
include "svc_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.om

/* ------------------------------------------------------------
   Operations on Volumes
   ------------------------------------------------------------*/

/**
 * Create Volume
 */
struct CtrlCreateBucket {
  /** Volume Specification */
  1: common.FDSP_CreateVolType  cv;
}

/**
 * Delete Volume
 */
struct CtrlDeleteBucket {
  /** Volume Specification */
  1:  common.FDSP_DeleteVolType dv;
}

/**
 * Modify Volume
 */
struct CtrlModifyBucket {
  /** Volume Specification */
  1:  common.FDSP_ModifyVolType mv;
}

/**
 * Test for Volume
 */
struct CtrlTestBucket {
  /** Volume Specification */
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

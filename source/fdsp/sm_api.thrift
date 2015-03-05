/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "sm_types.thrift"

include "common.thrift"
include "fds_service.thrift"
include "pm_service.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.dm

/* ------------------------------------------------------------
   Operations on Objects
   ------------------------------------------------------------*/

/**
 * Add volume reference to object.
 */
struct AddObjectRefMsg {
  /** List of objects to reference */
  1: list<common.FDS_ObjectIdType>  objIds;
  /** Source volume */
  2: i64                            srcVolId;
  /** Destination volume */
  3: i64                            destVolId;
}

/**
 * Reference addition response.
 */
struct AddObjectRefRspMsg {
}

/**
 * Delete object request message
 */
struct  DeleteObjectMsg {
  /** Volume referencing object. */
  1: i64                        volId;
  /** Object identifier */
  2: common.FDS_ObjectIdType    objId; 
}

/**
 * Delete object response message
 */
struct DeleteObjectRspMsg {
}

/**
 * Retrieve object data.
 */
struct GetObjectMsg {
  /** Volume identifier */
  1: required i64    			volume_id;
  /** Object identifier  */
  2: required common.FDS_ObjectIdType 	data_obj_id;
}

/**
 * Object retrieval response.
 */
struct GetObjectResp {
  /** Length of returned object. */
  1: i32              		data_obj_len;
  /** Object data. */
  2: binary           		data_obj;
}

/**
 * Put object request.
 */
struct PutObjectMsg {
  /** Volume identifier. */
   1: i64    			volume_id;
  /** Object identifier. */
   2: common.FDS_ObjectIdType 	data_obj_id;
  /** Object length.  */
   3: i32                      	data_obj_len;
  /** Object data.  */
   4: binary                   	data_obj;
}

/**
 * Put object response message
 */
struct PutObjectRspMsg {
}

/* ------------------------------------------------------------
   Operations on the Checker
   ------------------------------------------------------------*/

/**
 *
 */
struct CtrlNotifySMCheck {
  /**   */
    1: sm_types.SMCheckCmd SmCheckCmd;
}

/**
 *
 */
struct CtrlNotifySMCheckStatus {
}

/**
 *
 */
struct CtrlNotifySMCheckStatusResp {
  /**   */
    1: sm_types.SMCheckStatusType SmCheckStatus;
}

/* ------------------------------------------------------------
   Operations on the Scavenger
   ------------------------------------------------------------*/

/**
 *
 */
struct CtrlNotifyScavenger {
  /**   */
  1: sm_types.FDSP_ScavengerType   scavenger;
}

/**
 *
 */
struct CtrlQueryScavengerPolicy {
}

/**
 *
 */
struct CtrlQueryScavengerPolicyResp {
  /**   */
   1: i32                     dsk_threshold1;
  /**   */
   2: i32                     dsk_threshold2;
  /**   */
   3: i32                     token_reclaim_threshold;
  /**   */
   4: i32                     tokens_per_dsk;
}

/* ---------------------  CtrlScavengerProgressTypeId  --------------------------- */
/**
 *
 */
struct CtrlQueryScavengerProgress {
}

/**
 *
 */
struct CtrlQueryScavengerProgressResp {
  /**   */
   1: i32					  progress_pct;
}	   

/**
 *
 */
struct CtrlQueryScavengerStatus {
}

/**
 *
 */
struct CtrlQueryScavengerStatusResp {
   1: sm_types.FDSP_ScavengerStatusType 	status;
}

/* ---------------------  CtrlScavengerPolicyTypeId  --------------------------- */
/**
 *
 */
struct CtrlSetScavengerPolicy {
  /**   */
   1: i32					  dsk_threshold1;
  /**   */
   2: i32					  dsk_threshold2;
  /**   */
   3: i32					  token_reclaim_threshold;
  /**   */
   4: i32					  tokens_per_dsk;
}

/**
 *
 */
struct CtrlSetScavengerPolicyResp {
}

/* ------------------------  CtrlQueryScrubberStatusRespTypeId  ---------------------- */
/**
 *
 */
struct CtrlQueryScrubberStatusResp {
  /**   */
   1: sm_types.FDSP_ScavengerStatusType	scrubber_status;
}

/* ------------------------  CtrlQueryScrubberStatusTypeId  ------------------------- */
/**
 *
 */
struct CtrlQueryScrubberStatus {
}

/**
 *
 */
struct CtrlSetScrubberStatus {
  /**   */
   1: sm_types.FDSP_ScrubberStatusType		scrubber_status;
}

/**
 *
 */
struct CtrlSetScrubberStatusResp {
}


/* ------------------------------------------------------------
   Operations on Migrations
   ------------------------------------------------------------*/

/**
 * Migration complete for DLT version
 */
struct CtrlNotifyDLTClose {
  /** DLT version that started migration */
  1: sm_types.FDSP_DltCloseType   dlt_close;
}

/**
 * Asynchronous migration status?
 */
struct CtrlNotifyMigrationStatus {
  /**  Migration status */
  1: sm_types.FDSP_MigrationStatusType    status;
}

/**
 * Abort an in progress migration of tokens.
 */
struct CtrlNotifySMAbortMigration {
  /** DLT version that started migration */
  1: i64    DLT_version;
}

/**
 * Initiate a migration of tokens.
 */
struct CtrlNotifySMStartMigration {
  /** A list of tokens and their current source */
  1: list<sm_types.SMTokenMigrationGroup> migrations;
  /** The DLT version that initiated this migration */
  2: i64                            DLT_version;
}

/**
 * Message body to request Source SM to calculate and send delta set
 * from the metadata diff between the first rebalance msg and now.
 * In this second round, destination SM does not need to send filter set
 * as it does in the first round, because there are no active IO for this
 * token on the destination SM -- there could be active IO on the sorce SM.
 */
struct CtrlGetSecondRebalanceDeltaSet {
  /** unique id of executor on the destination SM */
  1: i64    executorID
}

/**
 * Rebalance request response
 */
struct CtrlGetSecondRebalanceDeltaSetRsp {
}

/**
 * A rebalance message to synchronize objects on the destination.
 */
struct CtrlObjectRebalanceDeltaSet {
  /** Unique id of executor on the destination SM */
  1: i64    executorID
  /** Sequence number of the delta set. */
  2: i64    seqNum
  /** boolean state to indicate that the whether this set is the last one or not. */
  3: bool   lastDeltaSet
  /** set of objects, which consists of data + metadata, to be applied  at the destination SM. */
  4: list<sm_types.CtrlObjectMetaDataPropagate>   objectToPropagate
}

/**
 * Message body to initiate the object rebalance between two
 * SMs.  The set of objects is sent from the destination SM to source
 * SM.  The set is filtered against the existing objects on SM, only
 * the "diff'ed" objects and meta data is sync'ed.
 */
struct CtrlObjectRebalanceFilterSet {
  /** Target DLT version for rebalance */
  1: i64    targetDltVersion
  /** DLT token to be rebalance */
  2: i32    tokenId
  /** Unique id of executor on the destination SM */
  3: i64    executorID
  /** Sequence number of the delta set */
  4: i64    seqNum
  /** True if this is the last message */
  5: bool   lastFilterSet
  /** Set of objects to be sync'ed */
  6: list<sm_types.CtrlObjectMetaDataSync>    objectsToFilter
}

/* ------------------------------------------------------------
   Other specified services
   ------------------------------------------------------------*/

/**
 * SM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service SMSvc extends pm_service.PlatNetSvc {
}

/**
 * Some service layer test thingy.
 */
service TestSMSvc {
  /**   */
    i32 associate(1: string myip, 2: i32 port);
  /**   */
    oneway void putObject(1: fds_service.AsyncHdr asyncHdr, 2: PutObjectMsg payload);
}

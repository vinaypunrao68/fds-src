/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"
include "fds_service.thrift"
include "FDSP.thrift"
include "pm_service.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.sm

/* -------------------- SM Enumerations -------------------- */

/**
 * Scavenger Commands
 */
enum FDSP_ScavengerCmd {
  /** Enable Automatic Garbage Collection */
  FDSP_SCAVENGER_ENABLE,
  /** Disable Automatic Garbage Collection */
  FDSP_SCAVENGER_DISABLE,
  /** Manually Trigger Garbage Collection */
  FDSP_SCAVENGER_START,
  /** Mannually Stop Garbage Collection */
  FDSP_SCAVENGER_STOP,
}

/**
 * Scavenger Status States
 */
enum FDSP_ScavengerStatusType {
   SCAV_ACTIVE       = 1,
   SCAV_INACTIVE     = 2,
   SCAV_DISABLED     = 3,
   SCAV_STOPPING     = 4
}

/**
 * Scrubber Status States
 */
enum FDSP_ScrubberStatusType {
   FDSP_SCRUB_ENABLE    = 1,
   FDSP_SCRUB_DISABLE   = 2
}

/**
 * Object reconciliation flag
 */
enum ObjectMetaDataReconcileFlags {
  /** No need to reconcile meta data. A new object. */
  OBJ_METADATA_NO_RECONCILE = 0,
  /** Need to reconcile meta data. Metadata has changed */
  OBJ_METADATA_RECONCILE    = 1,
  /** Overwrite existing metadata */
  OBJ_METADATA_OVERWRITE    = 2
}

/* -------------------- SM Variable Types -------------------- */

/**
 * Object volume association
 */
struct MetaDataVolumeAssoc {
  /** Object volume association */
  1: i64    volumeAssoc  
  /** reference count for volume association */
  2: i64    volumeRefCnt
}

/**
 * 
 */
struct SMTokenMigrationGroup {
  /**  */
  1: common.SvcUuid source;
  /**  */
  2: list<i32>      tokens;
}

/**
 * Object + Data + MetaData to be propogated to the destination SM from source SM
 */
struct CtrlObjectMetaDataPropagate
{
  /** Object ID */
  1: FDSP.FDS_ObjectIdType          objectID
  /** Reconcile action */
  2: ObjectMetaDataReconcileFlags   objectReconcileFlag
  /** User data */
  3: FDSP.FDSP_ObjectData           objectData
  /** Volume information */
  4: list<MetaDataVolumeAssoc>      objectVolumeAssoc
  /** Object refcnt */
  5: i64                            objectRefCnt
  /** Compression type for this object */
  6: i32                            objectCompressType
  /** Size of data after compression */
  7: i32                            objectCompressLen
  /** Object block size */
  8: i32                            objectBlkLen
  /** Object size */
  9: i32                            objectSize
  /** Object flag */
  10: i32                           objectFlags
  /** Object expieration time */
  11: i64                           objectExpireTime
}

/**
 * Object + subset of MetaData to determine if either the object or
 * associated MetaData (subset) needs sync'ing.
 */
struct CtrlObjectMetaDataSync 
{
  /** Object ID */
  1: FDSP.FDS_ObjectIdType      objectID
  /* RefCount of the object */
  2: i64                        objRefCnt
  /* volume information */
  3: list<MetaDataVolumeAssoc>  objVolAssoc
}

/**
 * Not sure...FIXME
 */
struct FDSP_DltCloseType {
  /** DLT version that started migration */
  1: i64 DLT_version;
}

/**
 * Migrations status type
 */
struct FDSP_MigrationStatusType {
  /** DLT version of migration */
  1: i64 DLT_version;
  /** ? FIXME */
  2: i32 context;
}

/* -------------------- Operations for Migration --------------------*/

/**
 * Migration complete for DLT version
 */
struct CtrlNotifyDLTClose {
  /** DLT version that started migration */
  1: FDSP_DltCloseType    dlt_close;
}

/**
 * Asynchronous migration status?
 */
struct CtrlNotifyMigrationStatus {
  /**  Migration status */
  1: FDSP_MigrationStatusType   status;
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
  1: list<SMTokenMigrationGroup>    migrations;
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
  1: i64 executorID
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
  1: i64 executorID
  /** Sequence number of the delta set. */
  2: i64      seqNum
  /** boolean state to indicate that the whether this set is the last one or not. */
  3: bool     lastDeltaSet
  /** set of objects, which consists of data + metadata, to be applied  at the destination SM. */
  4: list<CtrlObjectMetaDataPropagate> objectToPropagate
}

/**
 * Message body to initiate the object rebalance between two
 * SMs.  The set of objects is sent from the destination SM to source
 * SM.  The set is filtered against the existing objects on SM, only
 * the "diff'ed" objects and meta data is sync'ed.
 */
struct CtrlObjectRebalanceFilterSet {
  /** Target DLT version for rebalance */
  1: i64 targetDltVersion
  /** DLT token to be rebalance */
  2: FDSP.FDSP_Token              tokenId
  /** Unique id of executor on the destination SM */
  3: i64 executorID
  /** Sequence number of the delta set */
  4: i64 seqNum
  /** True if this is the last message */
  5: bool lastFilterSet
  /** Set of objects to be sync'ed */
  6: list<CtrlObjectMetaDataSync> objectsToFilter
}

/* -------------------- Operations for Objects --------------------*/

/* Copy objects from source volume to destination */
struct AddObjectRefMsg {
  /**   */
 1: list<FDSP.FDS_ObjectIdType> objIds,
  /**   */
 2: i64 srcVolId,
  /**   */
 3: i64 destVolId
}

struct AddObjectRefRspMsg {
}

/* Delete object request message */
struct  DeleteObjectMsg {
  /**   */
 1: i64 volId,
  /**   */
 2: FDSP.FDS_ObjectIdType objId, 
}

/* Delete object response message */
struct DeleteObjectRspMsg {
}

/* ----------------------- Scavenger --------------------------------------------- */

struct FDSP_ScavengerType {
  /**   */
  1: FDSP_ScavengerCmd  cmd
}

struct CtrlNotifyScavenger {
  /**   */
     1: FDSP_ScavengerType   scavenger;
}

struct CtrlQueryScavengerPolicy {
}

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
struct CtrlQueryScavengerProgress {
}

struct CtrlQueryScavengerProgressResp {
  /**   */
   1: i32					  progress_pct;
}	   

struct CtrlQueryScavengerStatus {
}

struct CtrlQueryScavengerStatusResp {
   1: FDSP_ScavengerStatusType 	status;
}

/* ---------------------  CtrlScavengerPolicyTypeId  --------------------------- */
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

struct CtrlSetScavengerPolicyResp {
}

/* ------------------------  CtrlQueryScrubberStatusRespTypeId  ---------------------- */
struct CtrlQueryScrubberStatusResp {
  /**   */
   1: FDSP_ScavengerStatusType	scrubber_status;
}

/* ------------------------  CtrlQueryScrubberStatusTypeId  ------------------------- */
struct CtrlQueryScrubberStatus {
}

struct CtrlSetScrubberStatus {
  /**   */
   1: FDSP_ScrubberStatusType		scrubber_status;
}

struct CtrlSetScrubberStatusResp {
}

/* ---------------------  SMCheck --------------------------------- */
/* command to enable/disable, start/stop smcheck */
enum SMCheckCmd {
    SMCHECK_DISABLE     = 0,
    SMCHECK_ENABLE      = 1,    
    SMCHECK_START       = 2,
    SMCHECK_STOP        = 3
}

/* bitmap flag of curent SMCheck status */
enum SMCheckStatusType {
    SMCHECK_STATUS_DISABLED  = 0x00,
    SMCHECK_STATUS_ENABLED   = 0x01,
    SMCHECK_STATUS_STARTED   = 0x10,
    SMCHECK_STATUS_STOPPED   = 0x20,
}

struct CtrlNotifySMCheck {
  /**   */
    1: SMCheckCmd SmCheckCmd;
}

struct CtrlNotifySMCheckStatus {
}

struct CtrlNotifySMCheckStatusResp {
  /**   */
    1: SMCheckStatusType SmCheckStatus;
}


/* Get object request message */
struct GetObjectMsg {
  /**   */
   1: required i64    			volume_id;
  /**   */
   2: required FDSP.FDS_ObjectIdType 	data_obj_id;
}

/* Get object response message */
struct GetObjectResp {
  /**   */
   1: i32              		data_obj_len;
  /**   */
   2: binary           		data_obj;
}

/* Put object request message */
struct PutObjectMsg {
  /**   */
   1: i64    			volume_id;
  /**   */
   2: FDSP.FDS_ObjectIdType 	data_obj_id;
  /**   */
   3: i32                      	data_obj_len;
  /**   */
   4: binary                   	data_obj;
}

/* Put object response message */
struct PutObjectRspMsg {
}

/* -------------------- Other specified services -------------------- */

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

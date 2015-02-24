/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"
include "fds_service.thrift"
include "FDSP.thrift"

namespace cpp FDS_ProtocolInterface

/**
 * SM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service SMSvc extends fds_service.PlatNetSvc {
}

/* ---------------------  CtrlScavengerStatusTypeId  --------------------------- */
enum FDSP_ScavengerStatusType {
   SCAV_ACTIVE       = 1,
   SCAV_INACTIVE     = 2,
   SCAV_DISABLED     = 3,
   SCAV_STOPPING     = 4
}

/* ------------------------  CtrlQueryScrubberStatusRespTypeId  ---------------------- */
enum FDSP_ScrubberStatusType {
   FDSP_SCRUB_ENABLE    = 1,
   FDSP_SCRUB_DISABLE   = 2
}

enum FDSP_ScavengerCmd {
  FDSP_SCAVENGER_ENABLE,     // enable automatic GC process
  FDSP_SCAVENGER_DISABLE,    // disable GC
  FDSP_SCAVENGER_START,      // start GC
  FDSP_SCAVENGER_STOP        // stop GC if it's running
}

struct FDSP_ScavengerType {
  1: FDSP_ScavengerCmd  cmd
}

/* ----------------------  CtrlStartMigration --------------------------- */
struct FDSP_DLT_Data_Type {
   1: bool dlt_type,
   2: binary dlt_data,
}

/* ---------------------- CtrlNotifySMStartMigration --------------------------- */
struct SMTokenMigrationGroup {
   1: common.SvcUuid                   source;
   2: list<i32>                 tokens;
}

/* ---------------------  CtrlNotifyDLTCloseTypeId  ---------------------------- */
struct FDSP_DltCloseType {
   1: i64 DLT_version
}

/* Object volume association */
struct MetaDataVolumeAssoc
{
    /* object volume association */
    1: i64 volumeAssoc  

    /* reference count for volume association */
    2: i64 volumeRefCnt
}

/* Object + Data + MetaData to be propogated to the destination SM from source SM */
struct CtrlObjectMetaDataPropagate
{
    /* Object ID */
    1: FDSP.FDS_ObjectIdType objectID

    /* If this flag is set, then the ObjectMetaDataProgate contains 
     * different data to be applied to the destination SM.
     *
     * TRUE -> Only objectVolumeAssoc and objectRefCnt are pertinent fields at this point.
     *         If true, these fields contains changes to the MetaData since the 
     *         object was migrated to the destination SM.
     *         objectData and other members are not set.
     * NOTE: If TRUE, treat ref_cnt (including volume association ref_cnt) as signed int64_t.
     *
     * FALSE -> All MetaData fields and objectData is set.  The MetaData and objectData
     *          can just be applied.
     *
     */
    2: bool isObjectMetaDataReconcile
    
    /* user data */
    3: FDSP.FDSP_ObjectData  objectData

    /* volume information */
    4: list<MetaDataVolumeAssoc> objectVolumeAssoc

    /* object refcnt */
    5: i64              objectRefCnt
    
    /* Compression type for this object */
    6: i32              objectCompressType

    /* Size of data after compression */
    7: i32              objectCompressLen

    /* Object block size */
    8: i32              objectBlkLen

    /* object size */
    9: i32              objectSize

    /* object flag */
    10: i32              objectFlags
    
    /* object expieration time */
    11: i64              objectExpireTime
}

/* Object + subset of MetaData to determine if either the object or
 * associated MetaData (subset) needs sync'ing.
 */
struct CtrlObjectMetaDataSync 
{
    /* Object ID */
    1: FDSP.FDS_ObjectIdType objectID

    /* RefCount of the object */
    2: i64              objRefCnt

    /* TODO(Sean):
     * There can be more fields in the MetaData that should be sync'ed,
     * but for now, RefCnt is only one we've identified.
     */
}

/* Copy objects from source volume to destination */
struct AddObjectRefMsg {
 1: list<FDSP.FDS_ObjectIdType> objIds,
 2: i64 srcVolId,
 3: i64 destVolId
}

struct AddObjectRefRspMsg {
}

/* Delete object request message */
struct  DeleteObjectMsg {
 1: i64 volId,
 2: FDSP.FDS_ObjectIdType objId, 
}

/* Delete object response message */
struct DeleteObjectRspMsg {
}

/* Message body to request Source SM to calculate and send delta set
 * from the metadata diff betweent the first rebalance msg and now.
 * In this second round, destination SM does not need to send filter set
 * as it does in the first round, because there are no active IO for this
 * token on the destination SM -- there could be active IO on the sorce SM.
 */
struct CtrlGetSecondRebalanceDeltaSet
{
   /* unique id of executor on the destination SM */
   1: i64 executorID
}

/* Get second rebalance delta set message response */
struct CtrlGetSecondRebalanceDeltaSetRsp {
}

struct CtrlNotifyDLTClose {
   1: FDSP_DltCloseType    dlt_close;
}

/* ---------------------  CtrlNotifyDLTUpdateTypeId  --------------------------- */
struct CtrlNotifyDLTUpdate {
   1: FDSP_DLT_Data_Type   dlt_data;
   2: i64                       dlt_version;
}

/* ----------------------  CtrlNotifyMigrationStatusTypeId  --------------------------- */
struct CtrlNotifyMigrationStatus {
   1: FDSP.FDSP_MigrationStatusType   status;
}

struct CtrlNotifyScavenger {
     1: FDSP_ScavengerType   scavenger;
}

struct CtrlNotifySMStartMigration {
   1: list<SMTokenMigrationGroup> migrations;
   2: i64							DLT_version;
}

struct CtrlObjectRebalanceDeltaSet
{
    /*
     * unique id of executor on the destination SM
     */
    1: i64 executorID

    /* sequence number of the delta set.  It's not important to handle
     * delta set sent from the source SM to the destination SM, but it's
     * important 
     */
    2: i64      seqNum

    /* boolean state to indicate that the whether this set is the last one
     * or noe.
     */
    3: bool     lastDeltaSet

    /* set of objects, which consists of data + metadata, to be applied 
     * at the destination SM.
     */
    4: list<CtrlObjectMetaDataPropagate> objectToPropagate
}

/* Message body to initiate the object rebalance between two
 * SMs.  The set of objects is sent from the destination SM to source
 * SM.  The set is filtered against the existing objects on SM, only
 * the "diff'ed" objects and meta data is sync'ed.
 */
struct CtrlObjectRebalanceFilterSet
{
    /* Target DLT version for rebalance */
    1: i64 targetDltVersion

    /* DLT token to be rebalance */
    2: FDSP.FDSP_Token              tokenId

    /* unique id of executor on the destination SM */
    3: i64 executorID

    /* sequence number */
    4: i64 seqNum

    /* true if this is the last message */
    5: bool lastFilterSet
    
    /* Set of objects to be sync'ed */
    6: list<CtrlObjectMetaDataSync> objectsToFilter
}

struct CtrlQueryScavengerPolicy {
}

struct CtrlQueryScavengerPolicyResp {
   1: i32                     dsk_threshold1;
   2: i32                     dsk_threshold2;
   3: i32                     token_reclaim_threshold;
   4: i32                     tokens_per_dsk;
}

/* ---------------------  CtrlScavengerProgressTypeId  --------------------------- */
struct CtrlQueryScavengerProgress {
}

struct CtrlQueryScavengerProgressResp {
   1: i32					  progress_pct;
}	   

struct CtrlQueryScavengerStatus {
}

struct CtrlQueryScavengerStatusResp {
   1: FDSP_ScavengerStatusType 	status;
}

/* ---------------------  CtrlScavengerPolicyTypeId  --------------------------- */
struct CtrlSetScavengerPolicy {
   1: i32					  dsk_threshold1;
   2: i32					  dsk_threshold2;
   3: i32					  token_reclaim_threshold;
   4: i32					  tokens_per_dsk;
}

struct CtrlSetScavengerPolicyResp {
}

/* ------------------------  CtrlQueryScrubberStatusRespTypeId  ---------------------- */
struct CtrlQueryScrubberStatusResp {
   1: FDSP_ScavengerStatusType	scrubber_status;
}

struct CtrlStartMigration {
   1: FDSP_DLT_Data_Type   dlt_data;
}

/* ------------------------  CtrlQueryScrubberStatusTypeId  ------------------------- */
struct CtrlQueryScrubberStatus {
}

struct CtrlSetScrubberStatus {
   1: FDSP_ScrubberStatusType		scrubber_status;
}

struct CtrlSetScrubberStatusResp {
}

/* ---------------------  CtrlNotifySMAbortMigrationTypeId  ---------------------------- */
struct CtrlNotifySMAbortMigration {
   1: i64  DLT_version;
}

/* Get object request message */
struct GetObjectMsg {
   1: required i64    			volume_id;
   2: required FDSP.FDS_ObjectIdType 	data_obj_id;
}

/* Get object response message */
struct GetObjectResp {
   1: i32              		data_obj_len;
   2: binary           		data_obj;
}

/* Put object request message */
struct PutObjectMsg {
   1: i64    			volume_id;
   2: FDSP.FDS_ObjectIdType 	data_obj_id;
   3: i32                      	data_obj_len;
   4: binary                   	data_obj;
}

/* Put object response message */
struct PutObjectRspMsg {
}

service TestSMSvc {
    i32 associate(1: string myip, 2: i32 port);
    oneway void putObject(1: fds_service.AsyncHdr asyncHdr, 2: PutObjectMsg payload);
}

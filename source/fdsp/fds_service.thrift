/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "FDSP.thrift"
include "snapshot.thrift"
include "common.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol

/*
 * Service/domain identifiers.
 */
struct SvcUuid {
    1: required i64           svc_uuid,
}

struct SvcID {
    1: required SvcUuid       svc_uuid,
    2: required string        svc_name,
}

struct SvcVer {
    1: required i16           ver_major,
    2: required i16           ver_minor,
}

struct DomainID {
    1: required SvcUuid       domain_id,
    2: required string        domain_name,
}

/*
 * List of all FDSP message types that passed between fds services.  Typically all these
 * types are for async messages.
 *
 * Process for adding new type:
 * 1. DON'T CHANGE THE ORDER.  ONLY APPEND.
 * 2. Add the type enum in appropriate service section.  Naming convection is
 *    Message type + "TypeId".  For example PutObjectMsg is PutObjectMsgTypeId.
 *    This helps us to use macros in c++.
 * 3. Add the type defition in the appropriate service .thrift file
 * 4. Register the serializer/deserializer in the appropriate service (.cpp/source) files
 */
enum  FDSPMsgTypeId {
    UnknownMsgTypeId                   = 0,

    /* Common Service Types */
    NullMsgTypeId                      = 10,
    EmptyMsgTypeId                     = 11,
    StatStreamMsgTypeId                = 12,

    /* Node/service event messages. */
    NodeSvcInfoTypeId                  = 1000,
    UuidBindMsgTypeId                  = 1001,
    DomainNodesTypeId                  = 1002,
    NodeInfoMsgTypeId                  = 1003,
    NodeInfoMsgListTypeId              = 1004,
    NodeQualifyTypeId                  = 1005,
    NodeUpgradeTypeId                  = 1006,
    NodeRollbackTypeId                 = 1007,
    NodeIntegrateTypeId                = 1008,
    NodeDeployTypeId                   = 1009,
    NodeFunctionalTypeId               = 1010,
    NodeDownTypeId                     = 1011,
    NodeEventTypeId                    = 1012,
    NodeWorkItemTypeId                 = 1013,
    PhaseSyncTypeId                    = 1014,

    /* Volume messages; common for AM, DM, SM. */
    CtrlNotifyVolAddTypeId             = 2020,
    CtrlNotifyVolRemoveTypeId          = 2021,
    CtrlNotifyVolModTypeId             = 2022,
    CtrlNotifySnapVolTypeId            = 2023,
    CtrlTierPolicyTypeId               = 2024,
    CtrlTierPolicyAuditTypeId          = 2025,

    /* SM messages. */
    CtrlStartMigrationTypeId           = 2040,
    CtrlNotifyScavengerTypeId          = 2041,
	CtrlQueryScavengerStatusTypeId	   = 2042,
	CtrlQueryScavengerStatusRespTypeId = 2043,
	CtrlQueryScavengerProgressTypeId   = 2044,
	CtrlQueryScavengerProgressRespTypeId = 2045,
	CtrlSetScavengerPolicyTypeId 	   = 2046,
	CtrlSetScavengerPolicyRespTypeId   = 2047,
	CtrlQueryScavengerPolicyTypeId     = 2048,
	CtrlQueryScavengerPolicyRespTypeId = 2049,
	CtrlQueryScrubberStatusTypeId	   = 2050,
	CtrlQueryScrubberStatusRespTypeId  = 2051,
	CtrlSetScrubberStatusTypeId		   = 2052,
	CtrlSetScrubberStatusRespTypeId	   = 2053,
	CtrlNotifyMigrationFinishedTypeId  = 2054,
	CtrlNotifyMigrationStatusTypeId	   = 2055,


    CtrlNotifyDLTUpdateTypeId          = 2060,
    CtrlNotifyDLTCloseTypeId           = 2061,
    CtrlNotifySMStartMigrationTypeId   = 2062,
    CtrlObjectRebalanceFilterSetTypeId = 2063,
    CtrlObjectRebalanceDeltaSetTypeId  = 2064,
    CtrlNotifySMAbortMigrationTypeId   = 2065,
    CtrlGetSecondRebalanceDeltaSetTypeId    = 2066,
    CtrlGetSecondRebalanceDeltaSetRspTypeId = 2067,

    /* DM messages. */
    CtrlNotifyPushDMTTypeId            = 2080,
    CtrlNotifyDMTCloseTypeId           = 2081,
    CtrlNotifyDMTUpdateTypeId          = 2082,

    /* OM-> AM messages. */
    CtrlNotifyBucketStatTypeId         = 2100,
    CtrlNotifyThrottleTypeId           = 2101,
    CtrlNotifyQoSControlTypeId         = 2102,

    /* AM-> OM */
    CtrlTestBucketTypeId	           = 3000,
    CtrlGetBucketStatsTypeId	       = 3001,
    CtrlCreateBucketTypeId             = 3002,
    CtrlDeleteBucketTypeId             = 3003,
    CtrlModifyBucketTypeId             = 3004,
    CtrlPerfStatsTypeId                = 3005,

    /* Svc -> OM */
    CtrlSvcEventTypeId                 = 9000,

    /* SM Type Ids*/
    GetObjectMsgTypeId 		= 10000, 
    GetObjectRespTypeId 	= 10001,
    PutObjectMsgTypeId		= 10002, 
    PutObjectRspMsgTypeId	= 10003,
	DeleteObjectMsgTypeId,
	DeleteObjectRspMsgTypeId,
    AddObjectRefMsgTypeId,
    AddObjectRefRspMsgTypeId,
    ShutdownMODMsgTypeId,
	

    /* DM Type Ids */
    QueryCatalogMsgTypeId = 20000,
    QueryCatalogRspMsgTypeId,
    StartBlobTxMsgTypeId,
    StartBlobTxRspMsgTypeId,
    UpdateCatalogMsgTypeId,
    UpdateCatalogRspMsgTypeId,
    UpdateCatalogOnceMsgTypeId,
    UpdateCatalogOnceRspMsgTypeId,
    SetBlobMetaDataMsgTypeId,
    SetBlobMetaDataRspMsgTypeId,
    DeleteCatalogObjectMsgTypeId,
    DeleteCatalogObjectRspMsgTypeId,
    GetBlobMetaDataMsgTypeId,
    GetBlobMetaDataRspMsgTypeId,
    SetVolumeMetaDataMsgTypeId,
    SetVolumeMetaDataRspMsgTypeId,
    GetVolumeMetaDataMsgTypeId,
    GetVolumeMetaDataRspMsgTypeId,
    CommitBlobTxMsgTypeId,
    CommitBlobTxRspMsgTypeId,
    AbortBlobTxMsgTypeId,
    AbortBlobTxRspMsgTypeId,
    GetBucketMsgTypeId,
    GetBucketRspMsgTypeId,
    DeleteBlobMsgTypeId,
    DeleteBlobRspMsgTypeId,
    ForwardCatalogMsgTypeId,
    ForwardCatalogRspMsgTypeId,
    VolSyncStateMsgTypeId,
    VolSyncStateRspMsgTypeId,
    StatStreamRegistrationMsgTypeId,
    StatStreamRegistrationRspMsgTypeId,
    StatStreamDeregistrationMsgTypeId,
    StatStreamDeregistrationRspMsgTypeId,
    CreateSnapshotMsgTypeId,
    CreateSnapshotRespMsgTypeId,
    DeleteSnapshotMsgTypeId,
    DeleteSnapshotRespMsgTypeId,
    CreateVolumeCloneMsgTypeId,
    CreateVolumeCloneRespMsgTypeId,
    GetDmStatsMsgTypeId,
    GetDmStatsMsgRespTypeId,
    ListBlobsByPatternMsgTypeId,
    ListBlobsByPatternRspMsgTypeId,
    ReloadVolumeMsgTypeId,
    ReloadVolumeRspMsgTypeId
}

/*
 * Generic response message format.
 */

/*
 * This message header is owned, controlled and set by the net service layer.
 * Application code treats it as opaque type.
 */
struct AsyncHdr {
    1: required i32           	msg_chksum;
    2: required FDSPMsgTypeId 	msg_type_id;
    3: required i32           	msg_src_id;
    4: required SvcUuid       	msg_src_uuid;
    5: required SvcUuid       	msg_dst_uuid;
    6: required i32           	msg_code;
    7: i64			rqSendStartTs;
    8: i64			rqSendEndTs;
    9: i64        		rqRcvdTs;
    10: i64        		rqHndlrTs;
    11: i64        		rspSerStartTs;	
    12:i64        		rspSendStartTs;	
    13:i64			rspRcvdTs;
}

/*
 * --------------------------------------------------------------------------------
 * Node endpoint/service registration handshake
 * --------------------------------------------------------------------------------
 */

/*
 * Uuid to physical location binding registration.
 */
struct UuidBindMsg {
    1: required SvcID                    svc_id,
    2: required string                   svc_addr,
    3: required i32                      svc_port,
    4: required SvcID                    svc_node,
    5: required string                   svc_auto_name,
    6: required FDSP.FDSP_MgrIdType      svc_type,
}

/*
 * Node storage capability message format.
 */
struct StorCapMsg {
    1: i32                    disk_iops_max,
    2: i32                    disk_iops_min,
    3: double                 disk_capacity,
    4: i32                    disk_latency_max,
    5: i32                    disk_latency_min,
    6: i32                    ssd_iops_max,
    7: i32                    ssd_iops_min,
    8: double                 ssd_capacity,
    9: i32                    ssd_latency_max,
    10: i32                   ssd_latency_min,
    11: i32                   ssd_count,
    12: i32                   disk_type,
    13: i32                   disk_count,
}

enum NodeSvcMask {
    NODE_SVC_SM       = 0x0001,
    NODE_SVC_DM       = 0x0002,
    NODE_SVC_AM       = 0x0004,
    NODE_SVC_OM       = 0x0008,
    NODE_SVC_GENERAL  = 0x1000
}

/*
 * Node registration message format.
 */
struct NodeInfoMsg {
    1: required UuidBindMsg   node_loc,
    2: required DomainID      node_domain,
    3: StorCapMsg    	      node_stor,
    4: required i32           nd_base_port,
    5: required i32           nd_svc_mask,
    6: required bool          nd_bcast,
}

struct NodeInfoMsgList {
    1: list<NodeInfoMsg>      nd_list,
}

enum ServiceStatus {
    SVC_STATUS_INVALID      = 0x0000,
    SVC_STATUS_ACTIVE       = 0x0001,
    SVC_STATUS_INACTIVE     = 0x0002,
    SVC_STATUS_IN_ERR       = 0x0004
}

/**
 * Service map info
 */
struct SvcInfo {
    1: required SvcID                    svc_id,
    2: required i32                      svc_port,
    3: required FDSP.FDSP_MgrIdType      svc_type,
    4: required ServiceStatus            svc_status,
    5: required string                   svc_auto_name,
}

/**
 * This becomes a control path message
 */
struct NodeSvcInfo {
    1: required SvcUuid                  node_base_uuid,
    2: i32                               node_base_port,
    3: string                            node_addr,
    4: string                            node_auto_name,
    5: FDSP.FDSP_NodeState               node_state;
    6: i32                               node_svc_mask,
    7: list<SvcInfo>                     node_svc_list,
}

struct DomainNodes {
    1: required DomainID                 dom_id,
    2: list<NodeSvcInfo>                 dom_nodes,
}

struct NodeVersion {
    1: required i32                      nd_fds_maj,
    2: required i32                      nd_fds_min,
    3: i32                               nd_fds_release,
    4: i32                               nd_fds_patch,
    5: string                            nd_java_ver,
}

/**
 * Qualify a node before admitting it to the domain.
 */
struct NodeQualify {
    1: required NodeInfoMsg              nd_info,
    2: required string                   nd_acces_token,
    3: NodeVersion                       nd_sw_ver,
    4: string                            nd_md5sum_pm,
    5: string                            nd_md5sum_sm,
    6: string                            nd_md5sum_dm,
    7: string                            nd_md5sum_am,
    8: string                            nd_md5sum_om,
}

/**
 * Notify node to upgrade/rollback SW version.
 */
struct NodeUpgrade {
    1: required DomainID                 nd_dom_id,
    2: required SvcUuid                  nd_uuid,
    3: NodeVersion                       nd_sw_ver,
    4: required FDSPMsgTypeId            nd_op_code,
    5: required string                   nd_md5_chksum,
    6: string                            nd_pkg_path,
}

struct NodeIntegrate {
    1: required DomainID                 nd_dom_id,
    2: required SvcUuid                  nd_uuid,
    3: bool                              nd_start_am,
    4: bool                              nd_start_dm,
    5: bool                              nd_start_sm,
    6: bool                              nd_start_om,
}

/**
 * Work item done by a node.
 */
struct NodeWorkItem {
    1: required i32                      nd_work_code,
    2: required DomainID                 nd_dom_id,
    3: required SvcUuid                  nd_from_svc,
    4: required SvcUuid                  nd_to_svc,
}

struct NodeDeploy {
    1: required DomainID                 nd_dom_id,
    2: required SvcUuid                  nd_uuid,
    3: list<NodeWorkItem>                nd_work_item,
}

struct NodeFunctional {
    1: required DomainID                 nd_dom_id,
    2: required SvcUuid                  nd_uuid,
    3: required FDSPMsgTypeId            nd_op_code,
    4: list<NodeWorkItem>                nd_work_item,
}

struct NodeDown {
    1: required DomainID                 nd_dom_id,
    2: required SvcUuid                  nd_uuid,
}

/**
 * Events emit from a node.
 */
struct NodeEvent {
    1: required DomainID                 nd_dom_id,
    2: required SvcUuid                  nd_uuid,
    3: required string                   nd_evt,
    4: string                            nd_evt_text,
}

/**
 * 2-Phase Commit message.
 */
struct PhaseSync {
    1: required SvcUuid                  ph_rcv_uuid,
    2: required i32                      ph_sync_evt,
    3: binary                            ph_data,
}

/*
 * --------------------------------------------------------------------------------
 * Common services
 * --------------------------------------------------------------------------------
 */

service BaseAsyncSvc {
    oneway void asyncReqt(1: AsyncHdr asyncHdr, 2: string payload);
    oneway void asyncResp(1: AsyncHdr asyncHdr, 2: string payload);
    AsyncHdr uuidBind(1: UuidBindMsg msg);
}

service PlatNetSvc extends BaseAsyncSvc {
    oneway void allUuidBinding(1: UuidBindMsg mine);
    oneway void notifyNodeActive(1: FDSP.FDSP_ActivateNodeType info);

    list<NodeInfoMsg> notifyNodeInfo(1: NodeInfoMsg info, 2: bool bcast);
    DomainNodes getDomainNodes(1: DomainNodes dom);
    NodeEvent   getSvcEvent(1: NodeEvent input);

    ServiceStatus getStatus(1: i32 nullarg);
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

/*
 * --------------------------------------------------------------------------------
 * Common messages
 * --------------------------------------------------------------------------------
 */
struct EmptyMsg {
}

/*
 * --------------------------------------------------------------------------------
 * Common Control Path Messages
 * --------------------------------------------------------------------------------
 */

/* ----------------------  CtrlNotifyVolAddTypeId  ----------------------------- */
struct CtrlNotifyVolAdd {
     1: FDSP.FDSP_VolumeDescType  vol_desc;   /* volume properties and attributes. */
     2: FDSP.FDSP_VolumeInfoType  vol_info;
     3: FDSP.FDSP_NotifyVolFlag   vol_flag;
}

/* ----------------------  CtrlNotifyVolRemoveTypeId  -------------------------- */
struct CtrlNotifyVolRemove {
     1: FDSP.FDSP_VolumeDescType  vol_desc;   /* volume properties and attributes. */
     2: FDSP.FDSP_NotifyVolFlag   vol_flag;
}

/* -----------------------  CtrlNotifyVolModTypeId  ---------------------------- */
struct CtrlNotifyVolMod {
     1: FDSP.FDSP_VolumeDescType  vol_desc;   /* volume properties and attributes. */
     2: FDSP.FDSP_NotifyVolFlag   vol_flag;
}

/* ----------------------  CtrlNotifySnapVolTypeId  ---------------------------- */
struct CtrlNotifySnapVol {
     1: FDSP.FDSP_VolumeDescType  vol_desc;   /* volume properties and attributes. */
     2: FDSP.FDSP_VolumeInfoType  vol_info;
     3: FDSP.FDSP_NotifyVolFlag   vol_flag;
}

/* ------------------------  CtrlTierPolicyTypeId  ----------------------------- */
struct CtrlTierPolicy {
     1: FDSP.FDSP_TierPolicy      tier_policy;
}

/* ----------------------  CtrlTierPolicyAuditTypeId  -------------------------- */
struct CtrlTierPolicyAudit {
     1: FDSP.FDSP_TierPolicyAudit tier_audit;
}

/* ----------------------  CtrlStartMigrationTypeId  --------------------------- */
struct CtrlStartMigration {
     1: FDSP.FDSP_DLT_Data_Type   dlt_data;
}

/* ----------------------  CtrlNotifyMigrationStatusTypeId  --------------------------- */
struct CtrlNotifyMigrationStatus {
     1: FDSP.FDSP_MigrationStatusType   status;
}

/* ---------------------  CtrlNotifyScavengerTypeId  --------------------------- */
struct CtrlNotifyScavenger {
     1: FDSP.FDSP_ScavengerType   scavenger;
}

struct CtrlNotifyQosControl {
     1: FDSP.FDSP_QoSControlMsgType qosctrl;
} 

/* ---------------------  CtrlScavengerStatusTypeId  --------------------------- */
enum FDSP_ScavengerStatusType {
	 SCAV_ACTIVE				  = 1,
	 SCAV_INACTIVE				  = 2,
	 SCAV_DISABLED				  = 3,
     SCAV_STOPPING                = 4	 
}

struct CtrlQueryScavengerStatus {
}

struct CtrlQueryScavengerStatusResp {
	   1: FDSP_ScavengerStatusType 	status;
}
/* ---------------------  ShutdownMODMsgTypeId  --------------------------- */
struct ShutdownMODMsg {
}

/* ---------------------  CtrlScavengerProgressTypeId  --------------------------- */
struct CtrlQueryScavengerProgress {
}

struct CtrlQueryScavengerProgressResp {
	   1: i32					  progress_pct;
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

struct CtrlQueryScavengerPolicy {
}

struct CtrlQueryScavengerPolicyResp {
       1: i32                     dsk_threshold1;
       2: i32                     dsk_threshold2;
       3: i32                     token_reclaim_threshold;
       4: i32                     tokens_per_dsk;
}

/* ------------------------  CtrlQueryScrubberStatusTypeId  ------------------------- */
struct CtrlQueryScrubberStatus {
}

/* ------------------------  CtrlQueryScrubberStatusRespTypeId  ---------------------- */
struct CtrlQueryScrubberStatusResp {
	 1: FDSP_ScavengerStatusType	scrubber_status;
}

/* ------------------------  CtrlQueryScrubberStatusRespTypeId  ---------------------- */
enum FDSP_ScrubberStatusType {
	 FDSP_SCRUB_ENABLE			 = 1,
	 FDSP_SCRUB_DISABLE			 = 2
}

struct CtrlSetScrubberStatus {
	 1: FDSP_ScrubberStatusType		scrubber_status;
}

struct CtrlSetScrubberStatusResp {
}

/* ---------------------  CtrlNotifyDLTUpdateTypeId  --------------------------- */
struct CtrlNotifyDLTUpdate {
     1: FDSP.FDSP_DLT_Data_Type   dlt_data;
     2: i64                       dlt_version;
}

/* ---------------------- CtrlNotifySMStartMigration --------------------------- */
struct SMTokenMigrationGroup {
     1: SvcUuid                   source;
     2: list<i32>                 tokens;
}

struct CtrlNotifySMStartMigration {
     1: list<SMTokenMigrationGroup> migrations;
	 2: i64							DLT_version;
}

/* ---------------------  CtrlNotifyDLTCloseTypeId  ---------------------------- */
struct CtrlNotifyDLTClose {
     1: FDSP.FDSP_DltCloseType    dlt_close;
}

/* ---------------------  CtrlNotifySMAbortMigrationTypeId  ---------------------------- */
struct CtrlNotifySMAbortMigration {
     1: i64  DLT_version;
}

/* ---------------------  CtrlNotifyPushDMTTypeId  ----------------------------- */
struct CtrlNotifyPushDMT {
     1: FDSP.FDSP_PushMeta        dmt_push;
}

/* ---------------------  CtrlNotifyDMTCloseTypeId  ---------------------------- */
struct CtrlNotifyDMTClose {
     1: FDSP.FDSP_DmtCloseType    dmt_close;
}

/* --------------------  CtrlNotifyDMTUpdateTypeId  ---------------------------- */
struct CtrlNotifyDMTUpdate {
     1: FDSP.FDSP_DMT_Type        dmt_data;
     2: i32                       dmt_version;
}

/* --------------------  CtrlNotifyBucketStatTypeId  --------------------------- */
struct CtrlNotifyBucketStat {
     1: FDSP.FDSP_BucketStatsRespType  bucket_stat;
}

/* ---------------------  CtrlNotifyThrottleTypeId  ---------------------------- */
struct CtrlNotifyThrottle {
     1: FDSP.FDSP_ThrottleMsgType      throttle;
}
struct CtrlNotifyQoSControl {
     1: FDSP.FDSP_QoSControlMsgType    qosctrl;
}
struct CtrlTestBucket {
     1: FDSP.FDSP_TestBucket           tbmsg;
}
struct CtrlGetBucketStats {
     1: FDSP.FDSP_GetDomainStatsType   gds;
     2: i32 req_cookie;
}
struct CtrlCreateBucket {
     1: FDSP.FDSP_CreateVolType       cv;
}
struct CtrlDeleteBucket {
    1:  FDSP.FDSP_DeleteVolType       dv;
}
struct CtrlModifyBucket {
    1:  FDSP.FDSP_ModifyVolType      mv;
}
struct CtrlPerfStats {
    1:  FDSP.FDSP_PerfstatsType     perfstats;
}

struct CtrlSvcEvent {
    1: required SvcUuid    evt_src_svc_uuid; // The svc uuid that this event targets
    2: required i32        evt_code;         // The error itself
    3: FDSPMsgTypeId       evt_msg_type_id;  // The msg that trigged this event (if any)
}


/* Registration for streaming stats */
struct StatStreamRegistrationMsg {
   1:i32 id,
   2:string url,
   3:string method
   4:SvcUuid dest,
   5:list<i64> volumes,
   6:i32 sample_freq_seconds,
   7:i32 duration_seconds,
}

struct StatStreamRegistrationRspMsg {
}

struct StatStreamDeregistrationMsg {
    1: i32 id;
}

struct StatStreamDeregistrationRspMsg {
}

/* snapshot message From OM => DM */
struct CreateSnapshotMsg {
    1:snapshot.Snapshot snapshot
}  

struct CreateSnapshotRespMsg {
    1:i64 snapshotId
}

struct DeleteSnapshotMsg {
    1:i64 snapshotId
}

struct DeleteSnapshotRespMsg {
    1:i64 snapshotId
}

struct ReloadVolumeMsg {
    1:i64 volId
}

struct CreateVolumeCloneMsg {
     1:i64 volumeId,
     2:i64 cloneId,
     3:string cloneName,
     4:i64 VolumePolicyId
}  

struct CreateVolumeCloneRespMsg {
     1:i64 cloneId
}

/*
 * --------------------------------------------------------------------------------
 * SM service specific messages
 * --------------------------------------------------------------------------------
 */
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
   2: i64                      	dlt_version;
   3: FDSP.FDS_ObjectIdType 	data_obj_id;
   4: i32                      	data_obj_len;
   5: binary                   	data_obj;
}

/* Put object response message */
struct PutObjectRspMsg {
}

/* Delete object request message */
struct  DeleteObjectMsg {
 1: i64 volId,
 2: FDSP.FDS_ObjectIdType objId, 
 3: i64 dlt_version
}

/* Delete object response message */
struct DeleteObjectRspMsg {
}

/* Copy objects from source volume to destination */
struct AddObjectRefMsg {
 1: list<FDSP.FDS_ObjectIdType> objIds,
 2: i64 srcVolId,
 3: i64 destVolId
}

struct AddObjectRefRspMsg {
}

/**
 * SM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service SMSvc extends PlatNetSvc {
}

/*
 * --------------------------------------------------------------------------------
 * DM service specific messages
 * --------------------------------------------------------------------------------
 */
 /* Query catalog request message */
struct QueryCatalogMsg {
   1: i64    			volume_id;
   2: string   			blob_name;		/* User visible name of the blob*/
   3: i64               start_offset;   /* Starting offset into the blob */
   4: i64               end_offset;     /* End offset into the blob */
   5: i64 		blob_version;   /* Version of the blob to query */
   6: FDSP.FDSP_BlobObjectList 	obj_list; 		/* List of object ids of the objects that this blob is being mapped to */
   7: FDSP.FDSP_MetaDataList 	meta_list;		/* sequence of arbitrary key/value pairs */
   8: i64                       byteCount;  /* Blob size */
}

// TODO(Rao): Use QueryCatalogRspMsg.  In current implementation we are using QueryCatalogMsg
// for response as well
/* Query catalog request message */
struct QueryCatalogRspMsg {
}

/* Update catalog request message */
struct UpdateCatalogMsg {
   1: i64    			volume_id;
   2: string 			blob_name; 	/* User visible name of the blob */
   3: i64                       blob_version; 	/* Version of the blob */
   4: i64                   	txId;
   5: FDSP.FDSP_BlobObjectList 	obj_list; 	/* List of object ids of the objects that this blob is being mapped to */
}

/* Update catalog response message */
struct UpdateCatalogRspMsg {
}

/* Update catalog once request message */
struct UpdateCatalogOnceMsg {
   1: i64    			volume_id;
   2: string 			blob_name; 	/* User visible name of the blob */
   3: i64                       blob_version; 	/* Version of the blob */
   4: i32 			blob_mode;
   5: i64                       dmt_version;
   6: i64                   	txId;
   7: FDSP.FDSP_BlobObjectList 	obj_list; 	/* List of object ids of the objects that this blob is being mapped to */
   8: FDSP.FDSP_MetaDataList 	meta_list;	/* sequence of arbitrary key/value pairs */
}

/* Update catalog once response message */
struct UpdateCatalogOnceRspMsg {
}

/* Forward catalog update request message */
struct ForwardCatalogMsg {
   1: i64    			volume_id;
   2: string 			blob_name; 	/* User visible name of the blob */
   3: i64                       blob_version; 	/* Version of the blob */
   4: FDSP.FDSP_BlobObjectList 	obj_list; 	/* List of object ids of the objects that this blob is being mapped to */
   5: FDSP.FDSP_MetaDataList 	meta_list;      /* sequence of arbitrary key/value pairs */
}

/* Forward catalog update response message */
struct ForwardCatalogRspMsg {
}

/* Start Blob  Transaction  request message */
struct StartBlobTxMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   4: i32 			blob_mode;
   5: i64 			txId;
   6: i64                       dmt_version;
}

/* start Blob traction response message */
struct StartBlobTxRspMsg {
}

/* Commit Blob  Transaction  request message */
struct CommitBlobTxMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   4: i64 			txId;
   5: i64                       dmt_version,
}

/* Commit Blob traction response message */
struct CommitBlobTxRspMsg {
}

/* Abort Blob  Transaction  request message */
struct AbortBlobTxMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   5: i64			txId;
}

/* Abort Blob traction response message */
struct AbortBlobTxRspMsg {
}
/* delete catalog object Transaction  request message */
struct DeleteCatalogObjectMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
}

struct DeleteCatalogObjectRspMsg {
}

/* get the list of blobs in volume Transaction  request message */
struct GetVolumeBlobListMsg {
   1: i64    			volume_id;
}

/* get the list of blobs in volume Transaction  request message */
struct SetBlobMetaDataMsg {
   1: i64    			volume_id;
   2: string 			blob_name;
   3: i64 			blob_version;
   4: FDSP.FDSP_MetaDataList    metaDataList; 
   5: i64                   	txId;
}

/* Set blob metadata request message */
struct SetBlobMetaDataRspMsg {
}

struct GetBlobMetaDataMsg {
  1: i64                       volume_id;
  2: string                    blob_name;
  3: i64                       blob_version;
  4: i64                       byteCount;
  5: FDSP.FDSP_MetaDataList    metaDataList;
}

struct GetBucketMsg {
  //request
  1: required i64              volume_id;
  2: i64                       startPos = 0;  
  3: i64                       count = 10000;
  4: string                    pattern = "";
  5: common.BlobListOrder      orderBy = 0;
  6: bool                      descending = false;
}

struct GetBucketRspMsg {
  //response
  1: required FDSP.BlobInfoListType     blob_info_list;
}

struct GetDmStatsMsg {
  1: i64                       volume_id;
  // response
  2: i64                       commitlog_size;
  3: i64                       extent0_size;
  4: i64                       extent_size;
  5: i64                       metadata_size;
}

struct GetVolumeMetaDataMsg {
  1: i64                        volume_id;
  //response
  2: FDSP.FDSP_VolumeMetaData   volume_meta_data;
}

struct DeleteBlobMsg {
  1: i64                       txId;
  2: i64                       volume_id;
  3: string                    blob_name;
  4: i64                       blob_version;
}

/* Volume sync state msg sent from source to destination DM */
struct VolSyncStateMsg
{
    1: i64        volume_id;
    2: bool       forward_complete;   /* true = forwarding done, false = second rsync done */
}

struct VolSyncStateRspMsg {
}

/**
 * time slot containing volume stats
 */
struct VolStatSlot {
    1: i64    rel_seconds;    // timestamp in seconds relative to start_timestamp
    2: binary slot_data;      // represents time slot of stats
}

/**
 * Volume's time-series of stats
 */
struct VolStatList {
    1: i64                  volume_id;  // volume id
    2: list<VolStatSlot>    statlist;   // list of time slots
}

/**
 * Stats from a service to DM for aggregation
 */
struct StatStreamMsg {
    1: i64                    start_timestamp;
    2: list<VolStatList>      volstats;
}

/**
 * DM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service DMSvc extends PlatNetSvc {
}

/**
 * AM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service AMSvc extends PlatNetSvc {
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

/* Message body to initiate the object rebalance between two
 * SMs.  The set of objects is sent from the destination SM to source
 * SM.  The set is filtered against the existing objects on SM, only
 * the "diff'ed" objects and meta data is sync'ed.
 */
struct CtrlObjectRebalanceFilterSet
{
    /* DLT token to be rebalance */
    1: FDSP.FDSP_Token              tokenId

    /* unique id of executor on the destination SM */
    2: i64 executorID

    /* sequence number */
    3: i64 seqNum

    /* true if this is the last message */
    4: bool lastFilterSet
    
    /* Set of objects to be sync'ed */
    5: list<CtrlObjectMetaDataSync> objectsToFilter
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
    11: i32              objectExpireTime    
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

#endif

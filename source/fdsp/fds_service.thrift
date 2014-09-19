/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
include "FDSP.thrift"
include "snapshot.thrift"

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

    CtrlNotifyDLTUpdateTypeId          = 2060,
    CtrlNotifyDLTCloseTypeId           = 2061,

    /* DM messages. */
    CtrlNotifyPushDMTTypeId            = 2080,
    CtrlNotifyDMTCloseTypeId           = 2081,
    CtrlNotifyDMTUpdateTypeId          = 2082,

    /* OM-> AM messages. */
    CtrlNotifyBucketStatTypeId         = 2100,
    CtrlNotifyThrottleTypeId           = 2101,
    CtrlNotifyQoSControlTypeId         = 2102,

    /* AM-> OM */
    CtrlTestBucketTypeId	       = 3000,
    CtrlGetBucketStatsTypeId	       = 3001,
    CtrlCreateBucketTypeId         = 3002,
    CtrlDeleteBucketTypeId         = 3003,
    CtrlModifyBucketTypeId         = 3004,
    CtrlPerfStatsTypeId            = 3005,

    /* SM Type Ids*/
    GetObjectMsgTypeId 		= 10000, 
    GetObjectRespTypeId 	= 10001,
    PutObjectMsgTypeId		= 10002, 
    PutObjectRspMsgTypeId	= 10003,
	DeleteObjectMsgTypeId,
	DeleteObjectRspMsgTypeId,
    AddObjectRefMsgTypeId,
    AddObjectRefRspMsgTypeId,

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
    CreateVolumeCloneRespMsgTypeId
}

/*
 * Generic response message format.
 */
 // TODO(Rao): Not sure if it's needed..Remove
struct RespHdr {
    1: required i32           status,
    2: required string        text,
 }

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

/*
 * State encoding:
 * 31             20           12           0
 * +--------------+-------------+-----------+
 * |     rsvd     | major state | sub state |
 * +----------------------------+-----------+
 */
const i32 om_state_shift     = 12

enum ServiceDeploymentState {
    ds_state_mask            = 0x7ff00000,

    /* We have nothing recorded in previous life. */
    ds_pristine              = 0x00100000,

    /* We were configured to belong to a local domain. */
    ds_in_ldomain            = 0x00200000,

    /* We activated service(s) running in empty shell. */
    ds_activated             = 0x00300000,

    /* We are deploying resources needed to start services. */
    ds_dply_resources        = 0x00400000,
    ds_dply_dlt              = 0x00400100,
    ds_dply_dmt              = 0x00400200,

    /* We are activating resources and their dependencies. */
    ds_activate_resources    = 0x00500000,
    ds_activate_dlt          = 0x00500100,
    ds_activate_dmt          = 0x00500200,

    /* We are at the working state, providing service(s). */
    ds_working               = 0x00600000,

    /* We are fully functional. */
    ds_full_functional       = 0x00700000,

    /* We are decommision resource/service. */
    ds_decommission          = 0x00800000,

    /* We shutdown resource/service. */
    ds_shutdown              = 0x00900000,

    /* We want to query the current state. */
    ds_current_state         = 0x00f00000,
}

enum ServiceRuntimeState {
    /* Common masks used in this state encoding. */
    rt_ack_mask             = 0x00080000,
    rt_timeout_mask         = 0x00040000,
    rt_state_mask           = 0x7ff00000,

    /* We are ready to do work. */
    rt_ready                = 0x00100000,

    /* We start service(s). */
    rt_start_svc            = 0x00200000,

    /* We are running as empty shell. */
    rt_empty_shell          = 0x00300000,

    /* We are integrating a new resource/service. */
    rt_integrate            = 0x00400000,
    rt_integrate_ack        = 0x00480000,
    rt_integrate_timeout    = 0x00440000,
    rt_integrate_node       = 0x00400001,

    /* We are deploying our resource(s). */
    rt_deploy_resources     = 0x00500000,
    rt_deploy_ack           = 0x00580000,
    rt_deploy_timeout       = 0x00540000,
    rt_update_dlt           = 0x00500010,
    rt_commit_dlt           = 0x00500020,
    rt_abort_dlt            = 0x00500040,
    rt_update_dmt           = 0x00500100,
    rt_commit_dmt           = 0x00500200,
    rt_abort_dmt            = 0x00500400,

    /* We are activating resource(s). */
    rt_activate_resources   = 0x00600000,
    rt_activate_ack         = 0x00680000,
    rt_activate_timeout     = 0x00640000,
    rt_activate_vols        = 0x00601000,

    /* We are working but not fully functional (e.g. full tolerant....) */
    rt_working              = 0x00700000,
    rt_working_pause        = 0x00700010,
    rt_working_resume       = 0x00700020,

    /* We are fully functional. */
    rt_full_functional      = 0x00800000,
    rt_funct_heartbeat      = 0x00810000,
    rt_funct_persist_state  = 0x00820000,
    rt_funct_persist_ack    = 0x00880000,

    /* We have a problem. */
    rt_problem              = 0x00900000,
    rt_peer_failure         = 0x00900010,
    rt_node_failure         = 0x00900100,
    rt_component_failure    = 0x00901000,

    /* We're synching resources. */
    rt_working_sync         = 0x00a00000,
    rt_working_sync_dlt     = 0x00a00010,
    rt_working_sync_dmt     = 0x00a00020,
    rt_working_sync_vols    = 0x00a00030,
    rt_working_sync_done    = 0x00a80000,
    rt_working_sync_timeout = 0x00a40000,

    /* We restart a service/resource. */
    rt_restart              = 0x00d00000,

    /* We disable a service/resource. */
    rt_disable              = 0x00e00000,

    /* We shutdown a service/resource. */
    rt_shutdown             = 0x00f00000,
    rt_stop_resources       = 0x00f01000,
    rt_release_resources    = 0x00f02000,
    rt_cleanup_resources    = 0x00f04000,
    rt_shutdown_done        = 0x00f80000,
    rt_shutdown_timeout     = 0x00f40000,
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
/*
    4: required ServiceRuntimeState      svc_runtime_state,
    6: required ServiceDeploymentState   svc_deployment_state,
    5: required string                   svc_auto_name
*/
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
/*
    5: ServiceRuntimeState               node_runtime_state,
    6: ServiceDeploymentState            node_deployment_state,
    7: i32                               node_svc_mask,
    8: list<SvcInfo>                     node_svc_list,
*/
}

struct DomainNodes {
    1: required AsyncHdr                 dom_hdr,
    2: required DomainID                 dom_id,
    3: list<NodeSvcInfo>                 dom_nodes,
/*
    1: required DomainID                 dom_id,
    2: list<NodeSvcInfo>                 dom_nodes,
*/
}

/*
 * --------------------------------------------------------------------------------
 * Common services
 * --------------------------------------------------------------------------------
 */

service BaseAsyncSvc {
    oneway void asyncReqt(1: AsyncHdr asyncHdr, 2: string payload);
    oneway void asyncResp(1: AsyncHdr asyncHdr, 2: string payload);
    RespHdr uuidBind(1: UuidBindMsg msg);
}

service PlatNetSvc extends BaseAsyncSvc {
    oneway void allUuidBinding(1: UuidBindMsg mine);
    oneway void notifyNodeActive(1: FDSP.FDSP_ActivateNodeType info);

    list<NodeInfoMsg> notifyNodeInfo(1: NodeInfoMsg info, 2: bool bcast);
    DomainNodes getDomainNodes(1: DomainNodes dom);

    ServiceStatus getStatus(1: i32 nullarg);
    map<string, i64> getCounters(1: string id);
    void resetCounters(1: string id);
    void setConfigVal(1:string id, 2:i64 value);
    void setFlag(1:string id, 2:i64 value);
    i64 getFlag(1:string id);
    map<string, i64> getFlags(1: i32 nullarg);
/*
    list<NodeInfoMsg> notifyNodeInfo(1: NodeInfoMsg info, 2: bool bcast);
    DomainNodes       getDomainNodes(1: DomainNodes dom);

    ServiceRuntimeState getStatus(1: i32 nullarg);
    map<string, i64> getCounters(1: string id);
    void resetCounters(1: string id);
    void setConfigVal(1:string id, 2:i64 value);

    void setFlag(1:string id, 2:i64 value);
    i64 getFlag(1:string id);
    map<string, i64> getFlags(1: i32 nullarg);

    void dumpFsm(1: string pattern);
*/
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

/* ---------------------  CtrlNotifyScavengerTypeId  --------------------------- */
struct CtrlNotifyScavenger {
     1: FDSP.FDSP_ScavengerType   scavenger;
}

struct CtrlNotifyQosControl {
     1: FDSP.FDSP_QoSControlMsgType qosctrl;
} 

/* ---------------------  CtrlNotifyDLTUpdateTypeId  --------------------------- */
struct CtrlNotifyDLTUpdate {
     1: FDSP.FDSP_DLT_Data_Type   dlt_data;
     2: i32                       dlt_version;
}

/* ---------------------  CtrlNotifyDLTCloseTypeId  ---------------------------- */
struct CtrlNotifyDLTClose {
     1: FDSP.FDSP_DltCloseType    dlt_close;
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
   2: i64                      	origin_timestamp;
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
 3: i64 origin_timestamp,  
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
   3: i64 			blob_version;        	/* Version of the blob to query */
   7: FDSP.FDSP_BlobObjectList 	obj_list; 		/* List of object ids of the objects that this blob is being mapped to */
   8: FDSP.FDSP_MetaDataList 	meta_list;		/* sequence of arbitrary key/value pairs */
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
  1: i64                       volume_id;
  2: i64                       startPos;  
  3: i64                       maxKeys;
  //response
  4: FDSP.BlobInfoListType     blob_info_list;
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
service AMSvc extends BaseAsyncSvc {
}


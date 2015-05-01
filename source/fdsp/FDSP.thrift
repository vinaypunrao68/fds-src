/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "common.thrift"

namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace * FDS_ProtocolInterface

enum fds_dmgr_txn_state {
  FDS_DMGR_TXN_STATUS_INVALID,
  FDS_DMGR_TXN_STATUS_OPEN,
  FDS_DMGR_TXN_STATUS_COMMITED,
  FDS_DMGR_TXN_STATUS_CANCELED
}

enum FDSP_MsgCodeType {
   FDSP_MSG_PUT_OBJ_REQ,
   FDSP_MSG_GET_OBJ_REQ,
   FDSP_MSG_DELETE_OBJ_REQ,
   FDSP_MSG_VERIFY_OBJ_REQ,
   FDSP_MSG_UPDATE_CAT_OBJ_REQ,
   FDSP_MSG_DELETE_BLOB_REQ,
   FDSP_MSG_QUERY_CAT_OBJ_REQ,
   FDSP_MSG_GET_VOL_BLOB_LIST_REQ,
   FDSP_MSG_OFFSET_WRITE_OBJ_REQ,
   FDSP_MSG_REDIR_READ_OBJ_REQ,
   FDSP_STAT_BLOB,
   FDSP_START_BLOB_TX,

   FDSP_MSG_PUT_OBJ_RSP,
   FDSP_MSG_GET_OBJ_RSP,
   FDSP_MSG_DELETE_OBJ_RSP,
   FDSP_MSG_VERIFY_OBJ_RSP,
   FDSP_MSG_UPDATE_CAT_OBJ_RSP,
   FDSP_MSG_DELETE_BLOB_RSP,
   FDSP_MSG_QUERY_CAT_OBJ_RSP,
   FDSP_MSG_GET_VOL_BLOB_LIST_RSP,
   FDSP_MSG_OFFSET_WRITE_OBJ_RSP,
   FDSP_MSG_REDIR_READ_OBJ_RSP,
   FDSP_MSG_GET_BUCKET_STATS_RSP,

   FDSP_MSG_CREATE_VOL,
   FDSP_MSG_MODIFY_VOL,
   FDSP_MSG_DELETE_VOL,
   FDSP_MSG_CREATE_POLICY,
   FDSP_MSG_MODIFY_POLICY,
   FDSP_MSG_DELETE_POLICY,
   FDSP_MSG_ATTACH_VOL_CMD,
   FDSP_MSG_DETACH_VOL_CMD,
   FDSP_MSG_REG_NODE,
   FDSP_MSG_TEST_BUCKET,

   FDSP_MSG_NOTIFY_VOL,
   FDSP_MSG_ATTACH_VOL_CTRL,
   FDSP_MSG_DETACH_VOL_CTRL,
   FDSP_MSG_NOTIFY_NODE_ADD,
   FDSP_MSG_NOTIFY_NODE_RMV,
   FDSP_MSG_NOTIFY_NODE_ACTIVE,
   FDSP_MSG_DMT_UPDATE,
   FDSP_MSG_DMT_CLOSE,
   FDSP_MSG_NODE_UPDATE,
   FDSP_MSG_NOTIFY_MIGRATION,
   FDSP_MSG_SCAVENGER_START,
   FDSP_MSG_PUSH_META,

   FDSP_MSG_SET_THROTTLE,
   FDSP_MSG_SET_QOS_CONTROL,
   FDSP_MSG_SNAP_VOL
}

enum FDSP_ResultType {
  FDSP_ERR_OK,
  FDSP_ERR_FAILED,
  FDSP_ERR_VOLUME_DOES_NOT_EXIST,
  FDSP_ERR_VOLUME_EXISTS,
  FDSP_ERR_BLOB_NOT_FOUND
  /* DEPRECATED: Don't add anthing here.  We will use Error from fds_err.h */
}

enum FDSP_ErrType {
  FDSP_ERR_OKOK,            /* not to conflict with result type, and protect when using fds_errno_t in msg_hdr.err_code */
  FDSP_ERR_SM_NO_SPACE,
  /* DEPRECATED: Don't add anthing here.  We will use Error from fds_err.h */
}

enum FDSP_VolNotifyType {
  FDSP_NOTIFY_DEFAULT,
  FDSP_NOTIFY_ADD_VOL,
  FDSP_NOTIFY_RM_VOL,
  FDSP_NOTIFY_MOD_VOL,
  FDSP_NOTIFY_SNAP_VOL
}

enum FDSP_ConsisProtoType {
  FDSP_CONS_PROTO_STRONG,
  FDSP_CONS_PROTO_WEAK,
  FDSP_CONS_PROTO_EVENTUAL
}

enum FDSP_AppWorkload {
    FDSP_APP_WKLD_TRANSACTION,
    FDSP_APP_WKLD_NOSQL,
    FDSP_APP_WKLD_HDFS,
    FDSP_APP_WKLD_JOURNAL_FILESYS,  // Ext3/ext4
    FDSP_APP_WKLD_FILESYS,  // XFS, other
    FDSP_APP_NATIVE_OBJS,  // Native object aka not going over http/rest apis
    FDSP_APP_S3_OBJS,  // Amazon S3 style objects workload
    FDSP_APP_AZURE_OBJS,  // Azure style objects workload
}

struct FDSP_PutObjType {
 1: common.FDS_ObjectIdType data_obj_id,
 2: i32              data_obj_len,
 3: i32              volume_offset, /* Offset inside the volume where the object resides */
 4: i32              dlt_version,
 5: binary           data_obj,
 6: binary           dlt_data,
}

struct FDSP_GetObjType {
 1: common.FDS_ObjectIdType data_obj_id,
 2: i32              data_obj_len,
 3: i32              dlt_version,
 4: binary           data_obj,
 5: binary           dlt_data,
}

struct  FDSP_DeleteObjType { /* This is a SH-->SM msg to delete the objectId */
 1: common.FDS_ObjectIdType data_obj_id,
 2: i32              dlt_version,
 3: i32              data_obj_len,
 4: binary           dlt_data,
}

struct FDSP_OffsetWriteObjType {
  1: common.FDS_ObjectIdType   data_obj_id_old,
  2: i32      data_obj_len,
  3: common.FDS_ObjectIdType   data_obj_id_new,
  4: i32      dlt_version,
  5: binary  data_obj,
  6: binary  dlt_data
}

struct FDSP_RedirReadObjType {
  1: common.FDS_ObjectIdType   data_obj_id_old,
  2: i32      data_obj_len,
  3: i32      data_obj_suboffset, /* Offset within the object where the actual data is modified */
  4: i32      data_obj_sublen,
  5: common.FDS_ObjectIdType   data_obj_id_new,
  6: i32      dlt_version,
  7: binary   data_obj,
  8: binary   dlt_data
}

struct FDSP_VerifyObjType {
  1: common.FDS_ObjectIdType   data_obj_id,
  2: i32      data_obj_len,
  3: binary  data_obj
}

struct FDSP_BlobDigestType {
#  1: i64 low,
#  2: i64 high
  1: binary  digest
}

/* Can be consolidated when apis and fdsp merge or whatever */
struct TxDescriptor {
       1: required i64 txId
       /* TODO(Andrew): Maybe add an op type (update/query)? */
}

struct  FDSP_DeleteCatalogType { /* This is a SH-->SM msg to delete the objectId */
  1: string   blob_name,       /* User visible name of the blob*/
  2: i64 blob_version, /* Version to delete */
  3: i32  dmt_version,
}

struct FDSP_ActivateAllNodesType {
  1: i32  domain_id,
  2: bool activate_sm,
  3: bool activate_dm,
  4: bool activate_am
}

struct FDSP_ActivateOneNodeType {
  1: i32        domain_id,
  2: common.FDSP_Uuid  node_uuid,
  3: bool       activate_sm,
  4: bool       activate_dm,
  5: bool       activate_am
}

struct FDSP_ActivateNodeType {
  1: common.FDSP_Uuid  node_uuid,
  2: string     node_name,          /* autogenerated name */
  3: bool       has_sm_service,     /* true if node runs sm service */
  4: bool       has_dm_service,     /* true if node runs dm service */
  5: bool       has_om_service,     /* true if node runs om service */
  6: bool       has_am_service      /* true if node runs am service */
}

typedef list<i64> Node_List_Type
typedef list<Node_List_Type> Node_Table_Type

struct FDSP_DLT_Type {
      1: i32 DLT_version,
      2: Node_Table_Type DLT,
}

struct FDSP_DLT_Resp_Type {
  1: i64 DLT_version
}

struct FDSP_DMT_Resp_Type {
  1: i64 DMT_version
}

struct FDSP_CreateDomainType {

  1: string 		 domain_name,
  2: i32			 domain_id,

}

struct FDSP_AttachVolCmdType {
  1: string		 vol_name, // Name of the volume to attach
  // double		 vol_uuid, // UUID of the volume being attached
  2: string		 node_id,  // Id of the hypervisor node where the volume should be attached
  3: i32			 domain_id,
}

struct FDSP_RemoveServicesType {
  1: string node_name, // Name of the node that contains services
  2: common.FDSP_Uuid   node_uuid,  // used if node name is not provided
  3: bool remove_sm,   // true if sm needs to be removed
  4: bool remove_dm,   // true to remove dm
  5: bool remove_am    // true to remove am
}

struct FDSP_ShutdownDomainType {
    1: i32 domain_id;
}

struct FDSP_GetVolInfoReqType {
 1: string vol_name,    /* name of the volume */
 3: i32    domain_id,
}

exception FDSP_VolumeNotFound {
  1: string message;
}

enum FDSP_NotifyVolFlag {
  FDSP_NOTIFY_VOL_NO_FLAG,
  FDSP_NOTIFY_VOL_CHECK_ONLY,  // for delete vol -- only check if objects in volume
  FDSP_NOTIFY_VOL_WILL_SYNC    // for create vol -- volume meta already exists on other node, will be synced
}

struct FDSP_AttachVolType {
  1: string 		 vol_name, /* Name of the volume */
  2: common.FDSP_VolumeDescType	 vol_desc, /* Volume properties and attributes */
}

struct FDSP_CreatePolicyType {
  1: string                 policy_name,  /* Name of the policy */
  2: common.FDSP_PolicyInfoType 	 policy_info,  /* Policy description */
}

struct FDSP_DeletePolicyType {
  1: string                 policy_name,  /* Name of the policy */
  2: i32                    policy_id,    /* policy id */
}

struct FDSP_ModifyPolicyType {
  1: string                 policy_name,  /* Name of the policy */
  2: i32                    policy_id,    /* policy id */
  3: common.FDSP_PolicyInfoType 	 policy_info,  /* Policy description */
}

struct FDSP_AnnounceDiskCapability {
  1: i32	node_iops_max,  /* iops suppported by */
  2: i32	node_iops_min,  /* iops suppported by */
  3: i64	disk_capacity, /* size of the disk  */
  4: i64	ssd_capacity, /* size of the ssd disks  */
  5: i32	disk_type,  /* disk type  */
}

struct FDSP_RegisterNodeType {
  1: common.FDSP_MgrIdType node_type,   /* Type of node - SM/DM/HV */
  2: string 	 node_name,      /* node indetifier - string */
  3: i32 	     domain_id,      /* domain indetifier */
  4: i64		 ip_hi_addr,     /* IP V6 high address */
  5: i64		 ip_lo_addr,     /* IP V4 address of V6 low address of the node */
  6: i32		 control_port,   /* Port number to contact for control messages */
  7: i32		 data_port,      /* Port number to send datapath requests */
  8: i32         migration_port, /*  Port for migration service */
  9: common.FDSP_Uuid   node_uuid;
  10: common.FDSP_Uuid  service_uuid;
  11: FDSP_AnnounceDiskCapability  disk_info, /* Add node capacity and other relevant fields here */
  12: string 	 node_root,      /* node root - string */
  13: i32         metasync_port, /*  Port for meta sync port service */
}

struct FDSP_BucketStatType {
  1: string             vol_name,
  2: double             performance,  /* average iops */
  3: double             sla,          /* minimum (guaranteed) iops */
  4: double             limit,        /* maximum iops */
  5: i32                rel_prio,     /* relative priority */
}

typedef list<FDSP_BucketStatType> FDSP_BucketStatListType

struct FDSP_BucketStatsRespType {
  1: string                    timestamp,          /* timestamp of the stats */
  2: FDSP_BucketStatListType   bucket_stats_list,  /* list of bucket stats */
}

struct FDSP_QueueStateType {

  1: i32 domain_id,
  2: i64  vol_uuid,
  3: i32 priority,
  4: double queue_depth, //current queue depth as a fraction of the total queue size. 0.5 means 50% full.

}

typedef list<FDSP_QueueStateType> FDSP_QueueStateListType

struct FDSP_NotifyQueueStateType {

  1: FDSP_QueueStateListType queue_state_list,

}

struct FDSP_MsgHdrType {
  1: FDSP_MsgCodeType     msg_code,

   // Message versioning for compatibility check, functionality changes
  2: i32 major_ver,   /* Major version number of this message */
  3: i32 minor_ver,   /* Minor version number of this message */
  4: i32 msg_id,      /* Message id to discard duplicate request & mai32ain causal order */
  5: i32 payload_len,
  6: i32 num_objects, /* Payload could contain more than one object */
  7: i32 frag_len,    /* Fragment Length  */
  8: i32 frag_num,    /* Fragment number for partial transfer */

  // Volume entity idenfiers
  9:  i32    tennant_id,      /* Tennant owning the Local-domain and  Storage  hypervisor */
  10: i32    local_domain_id, /* Local domain the volume in question bei64s */
  11: i64    glob_volume_id, /* Tennant owning the Local-domain and Storage hypervisor */
  12: string bucket_name, /* Bucket Name or Container Name for S3 or Azure entities */

 // Source and Destination Distributed s/w entities
  13: common.FDSP_MgrIdType  src_id,
  14: common.FDSP_MgrIdType  dst_id,
  15: i64             src_ip_hi_addr, /* IPv4 or IPv6 Address */
  16: i64             src_ip_lo_addr, /* IPv4 or IPv6 Address */
  17: i64             dst_ip_hi_addr, /* IPv4 or IPv6 address */
  18: i64             dst_ip_lo_addr, /* IPv4 or IPv6 address */
  19: i32             src_port,
  20: i32             dst_port,
  21: string          src_node_name, /* string identifying the source node that sent this request */

  // FDSP Result valid for response messages
  22: FDSP_ResultType result,
  23: string          err_msg,
  24: i32             err_code,

  // Checksum of the entire message including the payload/objects
  25: i32         req_cookie,
  26: i32         msg_chksum,
  27: string      payload_chksum,
  28: string      session_uuid,
  29: common.FDSP_Uuid   src_service_uuid,    /* uuid of service that sent this request */
  30: i64         origin_timestamp,
  31: i32         proxy_count,
  32: string      session_cache,  // this will be removed once we have  transcation journel  in DM
}

struct FDSP_VolMetaState
{
    1: i64        vol_uuid;
    2: bool       forward_done;   /* true means forwarding done, false means second rsync done */
}

/* Token type */
typedef i32 FDSP_Token

/* raw data for the object */
typedef string FDSP_ObjectData

struct FDSP_MigMsgHdrType
{
	/* Header */
	1: FDSP_MsgHdrType            base_header

	/* Id of the overall migration */
	2: string                     mig_id

	/* Id to identify unique migration between sender and receiver */
	3: string                     mig_stream_id;
}

/* Object id data pair */
struct FDSP_ObjectIdDataPair
{
	1: common.FDS_ObjectIdType  obj_id
	
	2: FDSP_ObjectData   data
}

/* Volume associations */
struct FDSP_ObjectVolumeAssociation
{
	1: common.FDSP_Uuid				vol_id
	2: i32						ref_cnt
}
typedef list<FDSP_ObjectVolumeAssociation> FDSP_ObjectVolumeAssociationList

/* Meta data for migration object */
/* DEPRECATED */
struct FDSP_MigrateObjectMetadata
{
    1: FDSP_Token              token_id
    2: common.FDS_ObjectIdType        object_id
    3: i32                     obj_len
    4: i64                     born_ts
    5: i64                     modification_ts
    6: FDSP_ObjectVolumeAssociationList associations
}

struct FDSP_GetObjMetadataReq {
 1: FDSP_MsgHdrType		header
 2: common.FDS_ObjectIdType 	obj_id
}

struct FDSP_GetObjMetadataResp {
 1: FDSP_MsgHdrType				header
 2: FDSP_MigrateObjectMetadata 	meta_data
}

/* Current state of tokens in an SM */
struct FDSP_TokenMigrationStats {
	/* Number of tokens for which migration is complete */
	1: i32			completed
	/* Number of token for which migration is in progress */
	2: i32          inflight
	/* Number of tokens for which migration is pending */
	3: i32          pending
}

service FDSP_SessionReq {
    oneway void AssociateRespCallback(1:string src_node_name) // Associate Response callback with DM/SM for this source node.
}

/* Response for establish session request */
struct FDSP_SessionReqResp {
    1: i32           status, // TODO: This should be fdsp error code
    2: string        sid,
}

service FDSP_Service {
	FDSP_SessionReqResp EstablishSession(1:FDSP_MsgHdrType fdsp_msg)
}

struct FDSP_CreateVolType {
  1: string                  vol_name,
  2: common.FDSP_VolumeDescType     vol_info, /* Volume properties and attributes */
}

struct FDSP_DeleteVolType {
  1: string 		 vol_name,  /* Name of the volume */
  // i64    		 vol_uuid,
  2: i32			 domain_id,
}

struct FDSP_ModifyVolType {
  1: string 		 vol_name,  /* Name of the volume */
  2: i64		 vol_uuid,
  3: common.FDSP_VolumeDescType	vol_desc,  /* New updated volume descriptor */
}

/*
 * From fdscli to OM (sync messages)
 */
service FDSP_ConfigPathReq {
  i32 CreateVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_CreateVolType crt_vol_req),
  i32 DeleteVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DeleteVolType del_vol_req),
  i32 ModifyVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ModifyVolType mod_vol_req),
  i32 SnapVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_CreateVolType snap_vol_req),
  i32 CreatePolicy(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_CreatePolicyType crt_pol_req),
  i32 DeletePolicy(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DeletePolicyType del_pol_req),
  i32 ModifyPolicy(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ModifyPolicyType mod_pol_req),
  i32 AttachVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_AttachVolCmdType atc_vol_req),
  i32 DetachVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_AttachVolCmdType dtc_vol_req),
  i32 AssociateRespCallback(1:i64 ident), // Associate Response callback ICE-object with DM/SM
  i32 CreateDomain(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_CreateDomainType crt_dom_req),
  i32 DeleteDomain(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_CreateDomainType del_dom_req),
  i32 SetThrottleLevel(1:FDSP_MsgHdrType fdsp_msg, 2:common.FDSP_ThrottleMsgType throttle_msg),
  common.FDSP_VolumeDescType GetVolInfo(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_GetVolInfoReqType vol_info_req) throws (1:FDSP_VolumeNotFound not_found),
  i32 RemoveServices(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_RemoveServicesType rm_node_req),
  i32 ActivateAllNodes(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ActivateAllNodesType act_node_req),
  i32 ActivateNode(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ActivateOneNodeType req),
  list<common.FDSP_Node_Info_Type> ListServices(1:FDSP_MsgHdrType fdsp_msg),
  list <common.FDSP_VolumeDescType> ListVolumes(1:FDSP_MsgHdrType fdsp_msg),
  i32 ShutdownDomain(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ShutdownDomainType dom_req)
}

/* Not needed.  But created for symemtry */
service FDSP_ConfigPathResp {
}

/*
 * Control/Native API config  messages from SM/DM/SH to OM
 * OM proxy
 */
service FDSP_OMControlPathReq {
  oneway void RegisterNode(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_RegisterNodeType reg_node_req),
}

service FDSP_OMControlPathResp {
  oneway void RegisterNodeResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_RegisterNodeType reg_node_rsp),
}

/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef __FDSP_H__
#define __FDSP_H__

include "common.thrift"

namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace * FDS_ProtocolInterface

struct FDS_ObjectIdType {
  1: string  digest,
  2: byte    conflict_id
}

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
   FDSP_MSG_DLT_UPDATE,
   FDSP_MSG_DLT_CLOSE,
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

enum FDSP_MgrIdType {
    FDSP_PLATFORM       = 0x0,
    FDSP_STOR_MGR       = 0x1,
    FDSP_DATA_MGR       = 0x2,
    FDSP_STOR_HVISOR    = 0x3,
    FDSP_ORCH_MGR       = 0x4,
    FDSP_CLI_MGR        = 0x5,
    FDSP_OMCLIENT_MGR   = 0x6,
    FDSP_MIGRATION_MGR  = 0x7,
    FDSP_PLATFORM_SVC   = 0x8,
    FDSP_METASYNC_MGR   = 0x9,
    FDSP_TEST_APP       = 0xa
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

enum FDSP_VolType {
  FDSP_VOL_S3_TYPE,
  FDSP_VOL_BLKDEV_TYPE
}

enum FDSP_MediaPolicy {
  FDSP_MEDIA_POLICY_UNSET,                /* only used by cli or other client on modify to not change existing policy */
  FDSP_MEDIA_POLICY_HDD,                  /* always on hdd */
  FDSP_MEDIA_POLICY_SSD,                  /* always on ssd */
  FDSP_MEDIA_POLICY_HYBRID,               /* either hdd or ssd, but prefer ssd */
  FDSP_MEDIA_POLICY_HYBRID_PREFCAP        /* either on hdd or ssd, but prefer hdd */
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
 1: FDS_ObjectIdType data_obj_id,
 2: i32              data_obj_len,
 3: i32              volume_offset, /* Offset inside the volume where the object resides */
 4: i32              dlt_version,
 5: binary           data_obj,
 6: binary           dlt_data,
}

struct FDSP_GetObjType {
 1: FDS_ObjectIdType data_obj_id,
 2: i32              data_obj_len,
 3: i32              dlt_version,
 4: binary           data_obj,
 5: binary           dlt_data,
}

struct  FDSP_DeleteObjType { /* This is a SH-->SM msg to delete the objectId */
 1: FDS_ObjectIdType data_obj_id,
 2: i32              dlt_version,
 3: i32              data_obj_len,
 4: binary           dlt_data,
}

struct FDSP_OffsetWriteObjType {
  1: FDS_ObjectIdType   data_obj_id_old,
  2: i32      data_obj_len,
  3: FDS_ObjectIdType   data_obj_id_new,
  4: i32      dlt_version,
  5: binary  data_obj,
  6: binary  dlt_data
}

struct FDSP_RedirReadObjType {
  1: FDS_ObjectIdType   data_obj_id_old,
  2: i32      data_obj_len,
  3: i32      data_obj_suboffset, /* Offset within the object where the actual data is modified */
  4: i32      data_obj_sublen,
  5: FDS_ObjectIdType   data_obj_id_new,
  6: i32      dlt_version,
  7: binary   data_obj,
  8: binary   dlt_data
}

struct FDSP_VerifyObjType {
  1: FDS_ObjectIdType   data_obj_id,
  2: i32      data_obj_len,
  3: binary  data_obj
}

struct FDSP_BlobDigestType {
#  1: i64 low,
#  2: i64 high
  1: binary  digest
}

struct FDSP_BlobObjectInfo {
 1: i64 offset,
 2: FDS_ObjectIdType data_obj_id,
 3: i64 size
 4: bool blob_end;
}

typedef list<FDSP_BlobObjectInfo> FDSP_BlobObjectList

struct FDSP_MetaDataPair {
 1: string key,
 2: string value,
}

typedef list <FDSP_MetaDataPair> FDSP_MetaDataList

struct FDSP_VolumeMetaData {
  1: i64 blobCount;
  2: i64 size; // in bytes
  3: i64 objectCount;
//  3: FDSP_MetaDataList metaData;
}

/* Can be consolidated when apis and fdsp merge or whatever */
struct TxDescriptor {
       1: required i64 txId
       /* TODO(Andrew): Maybe add an op type (update/query)? */
}

struct FDSP_UpdateCatalogType {
  1: string blob_name, /* User visible name of the blob */
  2: i64 blob_version, /* Version of the blob. Only used in response! */
  3: TxDescriptor txDesc, /* Transaction ID...can supersede other tx fields */
  4: i64 blob_size, /* Size of blob. Only use in  response! */
  5: i32 blob_mime_type, /* Encoding type of blob contents. */
  6: FDSP_BlobDigestType digest, /* Not sure...? */
  7: FDSP_BlobObjectList obj_list, /* List of object ids of the objects that this blob is being mapped to */
  8: FDSP_MetaDataList meta_list, /* sequence of arbitrary key/value pairs */
  9: i32 dm_transaction_id,   /* Transaction id */
  10: i32 dm_operation,        /* Transaction type = OPEN, COMMIT, CANCEL */
  11: i32 dmt_version,
}

/* Can be consolidated when apis and fdsp merge or whatever */
struct BlobDescriptor {
     1: required string name,
     2: required i64 byteCount,
     3: required map<string, string> metadata
}

struct FDSP_QueryCatalogType {

  1: string   blob_name,           /* User visible name of the blob*/
  2: i64 blob_version,             /* Version of the blob to query */
  3: i64 blob_size,
  4: i32 blob_mime_type,
  5: FDSP_BlobDigestType digest,
  6: FDSP_BlobObjectList obj_list, /* List of object ids of the objects that this blob is being mapped to */
  7: FDSP_MetaDataList meta_list,  /* sequence of arbitrary key/value pairs */
  8: i32  dm_transaction_id,   /* Transaction id */
  9: i32  dm_operation,        /* Transaction type = OPEN, COMMIT, CANCEL */
 10: i32  dmt_version,
}

struct  FDSP_DeleteCatalogType { /* This is a SH-->SM msg to delete the objectId */
  1: string   blob_name,       /* User visible name of the blob*/
  2: i64 blob_version, /* Version to delete */
  3: i32  dmt_version,
}

struct FDSP_BlobInfoType{
  1: string blob_name,
  2: i64 blob_size,
  4: required map<string, string> metadata
}

typedef list<FDSP_BlobInfoType> BlobInfoListType

/* A detailed list of blob stats. */
typedef list<BlobDescriptor> BlobDescriptorListType

struct FDSP_Uuid {
  1: i64          uuid,
}

enum FDSP_NodeState {
     FDS_Node_Up,
     FDS_Node_Down,
     FDS_Node_Rmvd,
     FDS_Node_Discovered,
     FDS_Start_Migration
}

struct FDSP_ActivateAllNodesType {
  1: i32  domain_id,
  2: bool activate_sm,
  3: bool activate_dm,
  4: bool activate_am
}

struct FDSP_ActivateOneNodeType {
  1: i32        domain_id,
  2: FDSP_Uuid  node_uuid,
  3: bool       activate_sm,
  4: bool       activate_dm,
  5: bool       activate_am
}

struct FDSP_ActivateNodeType {
  1: FDSP_Uuid  node_uuid,
  2: string     node_name,          /* autogenerated name */
  3: bool       has_sm_service,     /* true if node runs sm service */
  4: bool       has_dm_service,     /* true if node runs dm service */
  5: bool       has_om_service,     /* true if node runs om service */
  6: bool       has_am_service      /* true if node runs am service */
}

struct FDSP_Node_Info_Type {
  1: i32      node_id,
  2: FDSP_NodeState     node_state,
  3: FDSP_MgrIdType node_type, /* Type of node - SM/DM/HV */
  4: string 	 node_name,    /* node indetifier - string */
  5: i64		 ip_hi_addr, /* IP V6 high address */
  6: i64		 ip_lo_addr, /* IP V4 address of V6 low address of the node */
  7: i32		 control_port, /* Port number to contact for control messages */
  8: i32		 data_port, /* Port number to send datapath requests */
  9: i32                 migration_port, /* Migration service port */
  10: i64                node_uuid,      /* UUID of the node */
  11: i64                service_uuid,      /* UUID of the service */
  12: string 	         node_root,      /* node root - string */
  13: i32                 metasync_port, /* Migration service port */
}

typedef list<FDSP_Node_Info_Type> Node_Info_List_Type

typedef list<i64> Node_List_Type
typedef list<Node_List_Type> Node_Table_Type

struct FDSP_DLT_Type {
      1: i32 DLT_version,
      2: Node_Table_Type DLT,
}

struct FDSP_DMT_Type {
      1: i64 dmt_version,       /* DMT version */
      2: binary dmt_data        /* DMT table + meta in binary format */
      3: bool dmt_type,
}

struct FDSP_DLT_Data_Type {
	1: bool dlt_type,
    2: binary dlt_data,
}

struct FDSP_MigrationStatusType {
  1: i64 DLT_version,
  2: i32 context
}

struct FDSP_DltCloseType {
  1: i64 DLT_version
}

struct FDSP_DmtCloseType {
  1: i64 DMT_version
}

struct FDSP_DLT_Resp_Type {
  1: i64 DLT_version
}

struct FDSP_DMT_Resp_Type {
  1: i64 DMT_version
}

struct FDSP_VolumeDescType {
  1: required string            vol_name,  /* Name of the volume */
  2: i32                        tennantId,  // Tennant id that owns the volume
  3: i32                        localDomainId,  // Local domain id that owns vol
  4: required i64               volUUID,

  // Basic operational properties
  5: required FDSP_VolType      volType,
  6: i32                        maxObjSizeInBytes,
  7: required double            capacity,

  // Other policies
  8: i32                        volPolicyId,
  9: i32                        placementPolicy,  // Can change placement policy

  // volume policy details
  10: double                    iops_min, /* minimum (guaranteed) iops */
  11: double                    iops_max, /* maximum iops */
  12: i32                       rel_prio, /* relative priority */
  13: required FDSP_MediaPolicy mediaPolicy   /* media policy */

  14: bool                      fSnapshot,
  15: common.ResourceState      state,
  16: i64                       contCommitlogRetention,
  17: i64                       timelineTime,
  18: i64                       createTime,
  19: i32                       iops_guarantee, /* 0-100 percentage of max_iops that is guaranteed */
}


struct FDSP_CreateDomainType {

  1: string 		 domain_name,
  2: i32			 domain_id,

}

struct FDSP_GetDomainStatsType {
  1: i32			 domain_id,
}

struct FDSP_CreateVolType {

  1: string                  vol_name,
  2: FDSP_VolumeDescType     vol_info, /* Volume properties and attributes */

}

struct FDSP_TestBucket {
  1: string                 bucket_name,
  2: FDSP_VolumeDescType    vol_info, /* Bucket properties and attributes */
  3: bool                   attach_vol_reqd, /* Should OMgr send out an attach volume if the bucket is accessible, etc */
  4: string                 accessKeyId,
  5: string                 secretAccessKey,
}

struct FDSP_PolicyInfoType {
  1: string                 policy_name, /* Name of the policy */
  2: i32                    policy_id,   /* policy id */
  3: double                 iops_min,    /* minimum (guaranteed) iops */
  4: double                 iops_max,    /* maximum iops */
  5: i32                    rel_prio,    /* relative priority */
}

struct FDSP_DeleteVolType {
  1: string 		 vol_name,  /* Name of the volume */
  // i64    		 vol_uuid,
  2: i32			 domain_id,
}

struct FDSP_ModifyVolType {
  1: string 		 vol_name,  /* Name of the volume */
  2: i64		 vol_uuid,
  3: FDSP_VolumeDescType	 vol_desc,  /* New updated volume descriptor */
}

struct FDSP_AttachVolCmdType {
  1: string		 vol_name, // Name of the volume to attach
  // double		 vol_uuid, // UUID of the volume being attached
  2: string		 node_id,  // Id of the hypervisor node where the volume should be attached
  3: i32			 domain_id,
}

struct FDSP_RemoveServicesType {
  1: string node_name, // Name of the node that contains services
  2: FDSP_Uuid   node_uuid,  // used if node name is not provided
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

struct FDSP_NotifyVolType {
  1: FDSP_VolNotifyType 	 type,       /* Type of notify */
  2: string             	 vol_name,   /* Name of the volume */
  3: FDSP_VolumeDescType	 vol_desc,   /* Volume properties and attributes */
  4: FDSP_NotifyVolFlag          flag        /* see FDSP_NotifyVolFlag */
}

struct FDSP_AttachVolType {
  1: string 		 vol_name, /* Name of the volume */
  2: FDSP_VolumeDescType	 vol_desc, /* Volume properties and attributes */
}

struct FDSP_CreatePolicyType {
  1: string                 policy_name,  /* Name of the policy */
  2: FDSP_PolicyInfoType 	 policy_info,  /* Policy description */
}

struct FDSP_DeletePolicyType {
  1: string                 policy_name,  /* Name of the policy */
  2: i32                    policy_id,    /* policy id */
}

struct FDSP_ModifyPolicyType {
  1: string                 policy_name,  /* Name of the policy */
  2: i32                    policy_id,    /* policy id */
  3: FDSP_PolicyInfoType 	 policy_info,  /* Policy description */
}

struct FDSP_AnnounceDiskCapability {
  1: i32	disk_iops_max,  /* iops suppported by */
  2: i32	disk_iops_min,  /* iops suppported by */
  3: double	disk_capacity, /* size of the disk  */
  4: i32	disk_latency_max,  /* latency  */
  5: i32	disk_latency_min,  /* latency  */
  6: i32 	ssd_iops_max,  /* iops suppported by */
  7: i32 	ssd_iops_min,  /* iops suppported by */
  8: double	ssd_capacity, /* size of the disk  */
  9: i32	ssd_latency_max,  /* latency  */
  10: i32	ssd_latency_min,  /* latency  */
  11: i32	disk_type,  /* disk type  */
}

struct FDSP_RegisterNodeType {
  1: FDSP_MgrIdType node_type,   /* Type of node - SM/DM/HV */
  2: string 	 node_name,      /* node indetifier - string */
  3: i32 	     domain_id,      /* domain indetifier */
  4: i64		 ip_hi_addr,     /* IP V6 high address */
  5: i64		 ip_lo_addr,     /* IP V4 address of V6 low address of the node */
  6: i32		 control_port,   /* Port number to contact for control messages */
  7: i32		 data_port,      /* Port number to send datapath requests */
  8: i32         migration_port, /*  Port for migration service */
  9: FDSP_Uuid   node_uuid;
  10: FDSP_Uuid  service_uuid;
  11: FDSP_AnnounceDiskCapability  disk_info, /* Add node capacity and other relevant fields here */
  12: string 	 node_root,      /* node root - string */
  13: i32         metasync_port, /*  Port for meta sync port service */
}

struct FDSP_ThrottleMsgType {
  1: i32 	         domain_id, /* Domain this throttle message is meant for */
  2: double	         throttle_level, /* a real number between -10 and 10 */
  /*
     A throttle level of X.Y (e.g, 5.6) means we should
     1. throttle all traffic for priorities greater than X (priorities 6,7,8,9 for a 5.6 throttle level) to their guaranteed min rate,
     2. allow all traffic for priorities less than X (priorities 0,1,2,3,4 for a 5.6 throttle level) to go up till their max rate limit,
     3. throttle traffic for priority X to a rate = min_rate + Y/10 * (max_rate - min_rate).
        (A volume that has a min rate of 300 IOPS and max rate of 600 IOPS will be allowed to pump at 480 IOPS when throttle level is 5.6).

     A throttle level of 0 indicates all volumes should be limited at their min_iops rating.
     A negative throttle level of -X means all volumes should be throttled at (10-X)/10 of their min iops.
  */
}

/*
 * Message from OM to AM/SM/DM to help configure QoS control
 * For now using for OM setting total rate in AM based on current set of SMs
 * and their performance capabilities
 */
struct FDSP_QoSControlMsgType {
  1: i64   total_rate,   /* total rate in FDS ops/second */
}

struct FDSP_PerfStatType {
  1: i64   nios,     /* number of IOs in stat time i32erval  */
  2: i64   min_lat,  /* minimum latency */
  3: i64   max_lat,  /* maximum latency */
  4: double ave_lat,  /* average latency */

  5: i32 stat_type,     /* 0 - read/disk, 1 - write/disk, 2 - read/flash, 3 - write/flash, 5 - total
                      * Note that SH will only return stat_type 5 (for now) */
  6: i64 rel_seconds,  /* timestamp -- in seconds relative to FDSP_PerfstatsType::start_timestamp */
}

typedef list<FDSP_PerfStatType> FDSP_PerfStatListType

struct FDSP_VolPerfHistType {
  1:  i64 vol_uuid,
  2:  FDSP_PerfStatListType  stat_list,  /* list of performance stats (one or more time slots) for this volume */
}

typedef list<FDSP_VolPerfHistType> FDSP_VolPerfHistListType

struct FDSP_PerfstatsType {
  1: FDSP_MgrIdType            node_type, /* type of node - SM/SH */
  2: i32                       slot_len_sec, /* length of each stat time slot */
  3: string                    start_timestamp, /* to calc absolute timestamps of stats which contain relative timestamps */
  4: FDSP_VolPerfHistListType  vol_hist_list, /* list of performance histories of volumes */
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

struct FDSP_TierPolicy {
    1: i64           tier_vol_uuid,
    2: double          tier_domain_uuid,
    3: bool            tier_domain_policy,
    4: i32             tier_media,
    5: i32             tier_prefetch_algo,
    6: i64            tier_media_pct,
    7: i64            tier_interval_sec,
}

struct FDSP_TierPolicyAudit {
    1: i64          tier_vol_uuid,
    2: i64            tier_stat_min_iops,
    3: i64            tier_stat_max_iops,
    4: i64            tier_pct_ssd_iop,
    5: i64            tier_pct_hdd_iop,
    6: i64            tier_pct_ssd_capacity,
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
  13: FDSP_MgrIdType  src_id,
  14: FDSP_MgrIdType  dst_id,
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
  29: FDSP_Uuid   src_service_uuid,    /* uuid of service that sent this request */
  30: i64         origin_timestamp,
  31: i32         proxy_count,
  32: string      session_cache,  // this will be removed once we have  transcation journel  in DM
}

enum tier_prefetch_type_e
{
    PREFETCH_MRU,    /* most recently used to complement LRU */
    PREFETCH_RAND,   /* random eviction policy. */
    PREFETCH_ARC     /* arc algorithm */
}

enum tier_media_type_e
{
    TIER_MEIDA_NO_VAL = 0,
    TIER_MEDIA_DRAM,
    TIER_MEDIA_NVRAM,
    TIER_MEDIA_SSD,
    TIER_MEDIA_HDD,
    TIER_MEDIA_HYBRID,
    TIER_MEDIA_HYBRID_PREFCAP
}

struct tier_time_spec
{
    1: i32                      ts_sec;
    2: i32                      ts_min;
    /*
     * Hour encoding:
     *           0 : hourly.
     * 0x0000.001f : 1-24 regular hour of day.
     * 0x0000.0370 : 1-24 hour for range.
     * 0x0001.0000 : hour of day range cycle (e.g. 9-17h).
     * 0x0002.0000 : hour of day range interval.
     */
    3: i32                      ts_hour;
    /*
     * Day of month encoding:
     *           0 : daily.
     * 0x0000.001f : 1-31 regular day of month.
     * 0x0000.0370 : 1-31 day for range.
     * 0x0001.0000 : day of month range cycle.
     * 0x0002.0000 : day of month range interval.
     */
    4: i32                      ts_mday;
    /*
     * Day of week encoding:
     *           0 : daily.
     * 0x0000.0007 : 1-7 regular day of week.
     * 0x0000.0070 : 1-7 day for range.
     * 0x0001.0000 : day of week range cycle (e.g. M-F).
     * 0x0002.0000 : day of week range interval.
     */
    5: i32                      ts_wday;   /* day of the week. */
    /*
     * Month encoding:
     *           0 : monthly.
     * 0x0000.000f : 1-12 regular month.
     * 0x0000.00f0 : 1-12 month for range.
     * 0x0001.0000 : month range cycle (e.g. quarterly).
     * 0x0002.0000 : month range interval.
     */
    6: i32                      ts_mon;    /* month. */
    7: i32                      ts_year;   /* year. */
    8: i32                      ts_isdst;  /* daylight saving time. */
}

struct tier_pol_time_unit
{
    1: i64                    tier_vol_uuid;
    2: bool                     tier_domain_policy;
    3: i64                    tier_domain_uuid;
    4: tier_media_type_e        tier_media;
    5: tier_prefetch_type_e     tier_prefetch;
    6: i64                      tier_media_pct;
    7: tier_time_spec           tier_period;
}

struct tier_pol_audit
{
    1: i64                     tier_sla_min_iops;
    2: i64                     tier_sla_max_iops;
    3: i64                     tier_sla_min_latency;
    4: i64                     tier_sla_max_latency;
    5: i64                     tier_stat_min_iops;
    6: i64                     tier_stat_max_iops;
    7: i64                     tier_stat_avg_iops;
    8: i64                     tier_stat_min_latency;
    9: i64                     tier_stat_max_latency;
    10: i64                    tier_stat_avg_latency;

    11: i64                    tier_pct_ssd_iop;
    12: i64                    tier_pct_hdd_iop;
    13: i64                    tier_pct_ssd_capacity;
}

typedef list<i64> vol_List_Type
/* meta data  structure list of Volumes and  destination nodes */
struct FDSP_metaData
{
    /* Object Metadata */
    1: vol_List_Type  volList;
    2: FDSP_Uuid  node_uuid,
}

struct FDSP_VolMetaState
{
    1: i64        vol_uuid;
    2: bool       forward_done;   /* true means forwarding done, false means second rsync done */
}

typedef list<FDSP_metaData> FDSP_metaDataList
/* DM meta data migration request */
struct FDSP_PushMeta
{
     /* meta data */
     2: FDSP_metaDataList          metaVol;
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
	1: FDS_ObjectIdType  obj_id
	
	2: FDSP_ObjectData   data
}

/* Volume associations */
struct FDSP_ObjectVolumeAssociation
{
	1: FDSP_Uuid				vol_id
	2: i32						ref_cnt
}
typedef list<FDSP_ObjectVolumeAssociation> FDSP_ObjectVolumeAssociationList

/* Meta data for migration object */
/* DEPRECATED */
struct FDSP_MigrateObjectMetadata
{
    1: FDSP_Token              token_id
    2: FDS_ObjectIdType        object_id
    3: i32                     obj_len
    4: i64                     born_ts
    5: i64                     modification_ts
    6: FDSP_ObjectVolumeAssociationList associations
}

/* Complete data (metadata included) for migration object */
/* DEPRECATED */
struct FDSP_MigrateObjectData
{
    /* Object Metadata */
    1: FDSP_MigrateObjectMetadata meta_data

    /* Object data */
    2: FDSP_ObjectData            data
}

/* Collection of FDSP_MigrateObjectMetadata*/
/* DEPRECATED */
typedef list<FDSP_MigrateObjectMetadata> FDSP_MigrateObjectMetadataList

/* Collection of FDSP_MigrateObjectData */
/* DEPRECATED */
typedef list<FDSP_MigrateObjectData> FDSP_MigrateObjectList

struct FDSP_GetObjMetadataReq {
 1: FDSP_MsgHdrType		header
 2: FDS_ObjectIdType 	obj_id
}

struct FDSP_GetObjMetadataResp {
 1: FDSP_MsgHdrType				header
 2: FDSP_MigrateObjectMetadata 	meta_data
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

service FDSP_DataPathReq {
    oneway void GetObject(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_GetObjType get_obj_req),
    oneway void PutObject(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_PutObjType put_obj_req),
    oneway void DeleteObject(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DeleteObjType del_obj_req),
    oneway void OffsetWriteObject(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_OffsetWriteObjType offset_write_obj_req),
    oneway void RedirReadObject(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_RedirReadObjType redir_write_obj_req),

    /* Exposed for testing */
    oneway void GetObjectMetadata(1:FDSP_GetObjMetadataReq metadata_req),
    FDSP_TokenMigrationStats GetTokenMigrationStats(1:FDSP_MsgHdrType fdsp_msg)
}

service FDSP_DataPathResp {
    oneway void GetObjectResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_GetObjType get_obj_req),
    oneway void PutObjectResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_PutObjType put_obj_req),
    oneway void DeleteObjectResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DeleteObjType del_obj_req),
    oneway void OffsetWriteObjectResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_OffsetWriteObjType offset_write_obj_req),
    oneway void RedirReadObjectResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_RedirReadObjType redir_write_obj_req),
    /* Exposed for testing */
    oneway void GetObjectMetadataResp(1:FDSP_GetObjMetadataResp metadata_resp)
}

service FDSP_MetaDataPathReq {
    /* Using cleaner API convention. Just pass msg hdr for legacy compatability */
    oneway void StartBlobTx(1:FDSP_MsgHdrType fds_msg, 2:string volumeName, 3:string blobName, 4:TxDescriptor txId),
    oneway void UpdateCatalogObject(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_UpdateCatalogType cat_obj_req),
    oneway void QueryCatalogObject(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_QueryCatalogType cat_obj_req),
    oneway void DeleteCatalogObject(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DeleteCatalogType cat_obj_req),

    /* Using cleaner API convention. Just pass msg hdr for legacy compatability */
    oneway void StatBlob(1:FDSP_MsgHdrType fds_msg, 2:string volumeName, 3:string blobName)
    oneway void SetBlobMetaData(1:FDSP_MsgHdrType header, 2:string volumeName, 3:string blobName, 4:FDSP_MetaDataList metaDataList)
    oneway void GetBlobMetaData(1:FDSP_MsgHdrType header, 2:string volumeName, 3:string blobName)
    oneway void GetVolumeMetaData(1:FDSP_MsgHdrType header, 2:string volumeName)
}

service FDSP_MetaDataPathResp {
    /* Using cleaner API convention. Only success or error is returned. Done in msg hdr for legacy */
    oneway void StartBlobTxResp(1:FDSP_MsgHdrType fds_msg),
    oneway void UpdateCatalogObjectResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_UpdateCatalogType cat_obj_req),
    oneway void QueryCatalogObjectResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_QueryCatalogType cat_obj_req),
    oneway void DeleteCatalogObjectResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DeleteCatalogType cat_obj_req),

    /* Using cleaner API convention. Just pass msg hdr for legacy compatability */
    oneway void StatBlobResp(1:FDSP_MsgHdrType fds_msg, 2:BlobDescriptor blobDesc)
    oneway void SetBlobMetaDataResp(1:FDSP_MsgHdrType header, 2:string blobName)
    oneway void GetBlobMetaDataResp(1:FDSP_MsgHdrType header, 2:string blobName, 3:FDSP_MetaDataList metaDataList)
    oneway void GetVolumeMetaDataResp(1:FDSP_MsgHdrType header, 2:FDSP_VolumeMetaData volumeMeta)
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
  i32 SetThrottleLevel(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ThrottleMsgType throttle_msg),
  FDSP_VolumeDescType GetVolInfo(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_GetVolInfoReqType vol_info_req) throws (1:FDSP_VolumeNotFound not_found),
  i32 GetDomainStats(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_GetDomainStatsType get_stats_msg),
  i32 applyTierPolicy(1: tier_pol_time_unit policy),
  i32 auditTierPolicy(1: tier_pol_audit audit),
  i32 RemoveServices(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_RemoveServicesType rm_node_req),
  i32 ActivateAllNodes(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ActivateAllNodesType act_node_req),
  i32 ActivateNode(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ActivateOneNodeType req),
  i32 ScavengerCommand(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ScavengerType gc_req),
  list<FDSP_Node_Info_Type> ListServices(1:FDSP_MsgHdrType fdsp_msg),
  list <FDSP_VolumeDescType> ListVolumes(1:FDSP_MsgHdrType fdsp_msg),
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
  oneway void CreateBucket(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_CreateVolType crt_buck_req),
  oneway void DeleteBucket(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DeleteVolType del_buck_req),
  oneway void ModifyBucket(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ModifyVolType mod_buck_req),
  oneway void AttachBucket(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_AttachVolCmdType atc_buck_req),
  oneway void RegisterNode(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_RegisterNodeType reg_node_req),
  oneway void NotifyQueueFull(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_NotifyQueueStateType queue_state_info),
  oneway void NotifyPerfstats(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_PerfstatsType perf_stats_msg),
  oneway void TestBucket(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_TestBucket test_buck_msg),
  oneway void GetDomainStats(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_GetDomainStatsType get_stats_msg),
  oneway void NotifyMigrationDone(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_MigrationStatusType status_msg)
}

service FDSP_OMControlPathResp {
  oneway void CreateBucketResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_CreateVolType crt_buck_rsp),
  oneway void DeleteBucketResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DeleteVolType del_buck_rsp),
  oneway void ModifyBucketResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ModifyVolType mod_buck_rsp),
  oneway void AttachBucketResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_AttachVolCmdType atc_buck_req),
  oneway void RegisterNodeResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_RegisterNodeType reg_node_rsp),
  oneway void NotifyQueueFullResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_NotifyQueueStateType queue_state_rsp),
  oneway void NotifyPerfstatsResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_PerfstatsType perf_stats_rsp),
  oneway void TestBucketResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_TestBucket test_buck_rsp),
  oneway void GetDomainStatsResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_GetDomainStatsType get_stats_rsp),
  oneway void MigrationDoneResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_MigrationStatusType status_resp)
}

service FDSP_ControlPathReq {

  /* OM to SM/DM/SH control messages */

  oneway void NotifyAddVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_NotifyVolType not_add_vol_req),
  oneway void NotifyRmVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_NotifyVolType not_rm_vol_req),
  oneway void NotifyModVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_NotifyVolType not_mod_vol_req),
  oneway void NotifySnapVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_NotifyVolType not_snap_vol_req),
  oneway void AttachVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_AttachVolType atc_vol_req),
  oneway void DetachVol(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_AttachVolType dtc_vol_req),
  oneway void NotifyNodeAdd(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_Node_Info_Type node_info),
  oneway void NotifyNodeActive(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ActivateNodeType act_node_req),
  oneway void NotifyNodeRmv(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_Node_Info_Type node_info),
  oneway void NotifyDLTUpdate(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DLT_Data_Type dlt_info),
  oneway void NotifyDLTClose(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DltCloseType dlt_close),
  oneway void NotifyDMTUpdate(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DMT_Type dmt_info),
  oneway void NotifyDMTClose(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DmtCloseType dmt_close),
  oneway void PushMetaDMTReq(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_PushMeta push_meta_req),
  oneway void TierPolicy(1:FDSP_TierPolicy tier),
  oneway void TierPolicyAudit(1:FDSP_TierPolicyAudit audit),
  oneway void NotifyBucketStats(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_BucketStatsRespType buck_stats_msg),
  oneway void NotifyStartMigration(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DLT_Data_Type dlt_info),
  oneway void NotifyScavengerCmd(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_ScavengerType gc_info)
}

service FDSP_ControlPathResp {
  oneway void NotifyAddVolResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_NotifyVolType not_add_vol_resp),
  oneway void NotifyRmVolResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_NotifyVolType not_rm_vol_resp),
  oneway void NotifyModVolResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_NotifyVolType not_mod_vol_resp),
  oneway void NotifySnapVolResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_NotifyVolType not_snap_vol_resp),
  oneway void AttachVolResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_AttachVolType atc_vol_resp),
  oneway void DetachVolResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_AttachVolType dtc_vol_resp),
  oneway void NotifyNodeAddResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_Node_Info_Type node_info_resp),
  oneway void NotifyNodeRmvResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_Node_Info_Type node_info_resp),
  oneway void NotifyNodeActiveResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_Node_Info_Type node_info_resp),
  oneway void NotifyDLTUpdateResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DLT_Resp_Type dlt_resp),
  oneway void NotifyDLTCloseResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DLT_Resp_Type dlt_resp),
  oneway void NotifyDMTUpdateResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DMT_Resp_Type dmt_info_resp),
  oneway void NotifyDMTCloseResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_DMT_Resp_Type dmt_resp),
  oneway void PushMetaDMTResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_PushMeta push_meta_resp)
}

service FDSP_MetaSyncReq {
    oneway void PushMetaSyncReq(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_UpdateCatalogType push_meta_req)
    oneway void MetaSyncDone(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_VolMetaState vol_meta)

}

service FDSP_MetaSyncResp {
    oneway void PushMetaSyncResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_UpdateCatalogType push_meta_resp)
    oneway void MetaSyncDoneResp(1:FDSP_MsgHdrType fdsp_msg, 2:FDSP_VolMetaState vol_meta)
}

#endif

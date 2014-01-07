#ifndef __FDSP_H__
#define __FDSP_H__
namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace * FDS_ProtocolInterface


struct FDS_ObjectIdType {
  1: i64   hash_high,
  2: i64   hash_low,
  3: byte    conflict_id
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
   FDSP_MSG_DELETE_CAT_OBJ_REQ,
   FDSP_MSG_QUERY_CAT_OBJ_REQ,
   FDSP_MSG_GET_VOL_BLOB_LIST_REQ,
   FDSP_MSG_OFFSET_WRITE_OBJ_REQ,
   FDSP_MSG_REDIR_READ_OBJ_REQ,

   FDSP_MSG_PUT_OBJ_RSP,
   FDSP_MSG_GET_OBJ_RSP,
   FDSP_MSG_DELETE_OBJ_RSP,
   FDSP_MSG_VERIFY_OBJ_RSP,
   FDSP_MSG_UPDATE_CAT_OBJ_RSP,
   FDSP_MSG_DELETE_CAT_OBJ_RSP,
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
   FDSP_MSG_DLT_UPDATE,
   FDSP_MSG_DMT_UPDATE,
   FDSP_MSG_NODE_UPDATE

}

enum FDSP_MgrIdType {
    FDSP_STOR_MGR,
    FDSP_DATA_MGR,
    FDSP_STOR_HVISOR,
    FDSP_ORCH_MGR,
    FDSP_CLI_MGR
}

enum FDSP_ResultType {
  FDSP_ERR_OK,
  FDSP_ERR_FAILED,
  FDSP_ERR_VOLUME_DOES_NOT_EXIST,
  FDSP_ERR_VOLUME_EXISTS
}

enum FDSP_ErrType {
  FDSP_ERR_SM_NO_SPACE
}

enum FDSP_VolType {
  FDSP_VOL_S3_TYPE,
  FDSP_VOL_BLKDEV_TYPE,
  FDSP_VOL_BLKDEV_SSD_TYPE,
  FDSP_VOL_BLKDEV_DISK_TYPE,
  FDSP_VOL_BLKDEV_HYBRID_TYPE,
  FDSP_VOL_BLKDEV_HYBRID_PREFCAP_TYPE
}

enum FDSP_VolNotifyType {
  FDSP_NOTIFY_DEFAULT,
  FDSP_NOTIFY_ADD_VOL,
  FDSP_NOTIFY_RM_VOL,
  FDSP_NOTIFY_MOD_VOL
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
  1: FDS_ObjectIdType   data_obj_id,
  2: i32      data_obj_len,
  3: i32      volume_offset, /* Offset inside the volume where the object resides */
  4: string data_obj
}

struct FDSP_GetObjType {
  1: FDS_ObjectIdType   data_obj_id,
  2: i32      data_obj_len,
  3: string  data_obj
}

struct  FDSP_DeleteObjType { /* This is a SH-->SM msg to delete the objectId */
  1: FDS_ObjectIdType   data_obj_id,
  2: i32      data_obj_len,
}


struct FDSP_OffsetWriteObjType {
  1: FDS_ObjectIdType   data_obj_id_old,
  2: i32      data_obj_len,
  3: FDS_ObjectIdType   data_obj_id_new,
  4: string  data_obj
}


struct FDSP_RedirReadObjType {
  1: FDS_ObjectIdType   data_obj_id_old,
  2: i32      data_obj_len,
  3: i32      data_obj_suboffset, /* Offset within the object where the actual data is modified */
  4: i32      data_obj_sublen,
  5: FDS_ObjectIdType   data_obj_id_new,
  6: string   data_obj
} 


struct FDSP_VerifyObjType {
  1: FDS_ObjectIdType   data_obj_id,
  2: i32      data_obj_len,
  3: string  data_obj
}

struct FDSP_BlobDigestType {
  1: i64 low,
  2: i64 high
}

struct FDSP_BlobObjectInfo {
 1: i64 offset,
 2: FDS_ObjectIdType data_obj_id,
 3: i64 size     
}

typedef list<FDSP_BlobObjectInfo> FDSP_BlobObjectList

struct FDSP_MetaDataPair {
 1: string key,
 2: string value,
}

typedef list <FDSP_MetaDataPair> FDSP_MetaDataList

struct FDSP_UpdateCatalogType {
  1: string   blob_name,       /* User visible name of the blob*/
  2: i64 blob_size,
  3: i32 blob_mime_type,
  4: FDSP_BlobDigestType digest,
  5: FDSP_BlobObjectList   obj_list,         /* List of object ids of the objects that this blob is being mapped to */
  6: FDSP_MetaDataList meta_list, /* sequence of arbitrary key/value pairs */

  7: i32      dm_transaction_id,  /* Transaction id */
  8: i32      dm_operation,       /* Transaction type = OPEN, COMMIT, CANCEL */
}

struct FDSP_QueryCatalogType {

  1: string   blob_name,       /* User visible name of the blob*/
  2: i64 blob_size,
  3: i32 blob_mime_type,
  4: FDSP_BlobDigestType digest,
  5: FDSP_BlobObjectList   obj_list,         /* List of object ids of the objects that this blob is being mapped to */
  6: FDSP_MetaDataList meta_list, /* sequence of arbitrary key/value pairs */

  7: i32      dm_transaction_id,  /* Transaction id */
  8: i32      dm_operation,       /* Transaction type = OPEN, COMMIT, CANCEL */
}

struct  FDSP_DeleteCatalogType { /* This is a SH-->SM msg to delete the objectId */
  1: string   blob_name,       /* User visible name of the blob*/
}

struct FDSP_BlobInfoType{
  1: string blob_name,
  2: i64 blob_size,
  3: i32 mime_type,
}

typedef list<FDSP_BlobInfoType> BlobInfoListType

const i64 blob_list_iterator_begin = 0
const i64 blob_list_iterator_end = 0xffffffff

struct FDSP_GetVolumeBlobListReqType {
  // take volUUID from the header
  1: i32 max_blobs_to_return, 
  2: i64 iterator_cookie, //set to "0" the first time and store and return for successive iterations
}

struct FDSP_GetVolumeBlobListRespType {
  1: i32 num_blobs_in_resp,
  2: bool end_of_list, // true if there are no more blobs to return
  3: i64 iterator_cookie, // store and return this for the next iteration.
  4: BlobInfoListType blob_info_list, // list of blob_info structures.  
}


enum FDSP_NodeState {
     FDS_Node_Up,
     FDS_Node_Down,
     FDS_Node_Rmvd
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
}

typedef list<FDSP_Node_Info_Type> Node_Info_List_Type

typedef list<i32> Node_List_Type
typedef list<Node_List_Type> Node_Table_Type


struct FDSP_DLT_Type {
      1: i32 DLT_version,
      2: Node_Table_Type DLT,
}

struct FDSP_DMT_Type {
      1: i32 DMT_version,
      2: Node_Table_Type DMT,
}


struct FDSP_VolumeInfoType {

  1: string 		 vol_name,  /* Name of the volume or bucket*/
  2: i32 	 		 tennantId,  // Tennant id that owns the volume
  3: i32    		 localDomainId,  // Local domain id that owns vol
  4: i32	 		 globDomainId,
  // double	 	 volUUID,

// Basic operational properties

  5: FDSP_VolType		 volType,		 		 
  6: double        	 capacity,
  7: double        	 maxQuota,  // Quota % of capacity tho should alert

// Consistency related properties

  8: i32        		 defReplicaCnt,  // Number of replicas reqd for this volume
  9: i32        		 defWriteQuorum,  // Quorum number of writes for success
  10: i32        		 defReadQuorum,  // This will be 1 for now
  11: FDSP_ConsisProtoType 	 defConsisProtocol,  // Read-Write consistency protocol

// Other policies

  12: i32        		 volPolicyId,
  13: i32         		 archivePolicyId,
  14: i32        		 placementPolicy,  // Can change placement policy
  15: FDSP_AppWorkload     	 appWorkload,

  16: i32         		 backupVolume,  // UUID of backup volume

}

struct FDSP_VolumeDescType {

  1: string 		 vol_name,  /* Name of the volume */
  2: i32 	 		 tennantId,  // Tennant id that owns the volume
  3: i32    		 localDomainId,  // Local domain id that owns vol
  4: i32	 		 globDomainId,
  5: double	 	 volUUID,

// Basic operational properties

  6: FDSP_VolType		 volType,		 		 
  7: double        	 capacity,
  8: double        	 maxQuota,  // Quota % of capacity tho should alert

// Consistency related properties

  9: i32        		 defReplicaCnt,  // Number of replicas reqd for this volume
  10: i32        		 defWriteQuorum,  // Quorum number of writes for success
  11: i32        		 defReadQuorum,  // This will be 1 for now
  12: FDSP_ConsisProtoType 	 defConsisProtocol,  // Read-Write consistency protocol

// Other policies

  13: i32        		 volPolicyId,
  14: i32         		 archivePolicyId,
  15: i32        		 placementPolicy,  // Can change placement policy
  16: FDSP_AppWorkload     	 appWorkload,

  17: i32         		 backupVolume,  // UUID of backup volume

// volume policy details 
  18: double                 iops_min, /* minimum (guaranteed) iops */
  19: double                 iops_max, /* maximum iops */
  20: i32                    rel_prio, /* relative priority */
}

struct FDSP_CreateDomainType {

  1: string 		 domain_name,
  2: i32			 domain_id,

}

struct FDSP_GetDomainStatsType {
  1: i32			 domain_id,
}

struct FDSP_CreateVolType {

  1: string 		 vol_name,
  2: FDSP_VolumeInfoType 	 vol_info, /* Volume properties and attributes */

}

struct FDSP_TestBucket {
  1: string 		 bucket_name,
  2: FDSP_VolumeInfoType 	 vol_info, /* Bucket properties and attributes */
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
  // double    		 vol_uuid,
  2: i32			 domain_id,
}

struct FDSP_ModifyVolType {
  1: string 		 vol_name,  /* Name of the volume */
  2: double		 vol_uuid,
  3: FDSP_VolumeDescType	 vol_desc,  /* New updated volume descriptor */
}

struct FDSP_AttachVolCmdType {
  1: string		 vol_name, // Name of the volume to attach
  // double		 vol_uuid, // UUID of the volume being attached
  2: string		 node_id,  // Id of the hypervisor node where the volume should be attached
  3: i32			 domain_id,
}


struct FDSP_GetVolInfoReqType {
 1: string vol_name,
 2: i32 domain_id,
}

struct FDSP_GetVolInfoRespType {
 1: string vol_name,
 2: FDSP_VolumeDescType vol_desc,
}

struct FDSP_NotifyVolType {
  1: FDSP_VolNotifyType 	 type,       /* Type of notify */
  2: string             	 vol_name,   /* Name of the volume */
  3: FDSP_VolumeDescType	 vol_desc,   /* Volume properties and attributes */
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
  1: FDSP_MgrIdType node_type, /* Type of node - SM/DM/HV */
  2: string 	 node_name,    /* node indetifier - string */
  3: i32 	     domain_id,    /* domain indetifier */
  4: i64		 ip_hi_addr, /* IP V6 high address */
  5: i64		 ip_lo_addr, /* IP V4 address of V6 low address of the node */
  6: i32		 control_port, /* Port number to contact for control messages */
  7: i32		 data_port, /* Port number to send datapath requests */
  8: FDSP_AnnounceDiskCapability  disk_info, /* Add node capacity and other relevant fields here */
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
  1: double vol_uuid,
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
  1: double             vol_uuid,
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
  2: double vol_uuid,
  3: i32 priority,
  4: double queue_depth, //current queue depth as a fraction of the total queue size. 0.5 means 50% full.
  
}

struct FDSP_TierPolicy {
    1: double          tier_vol_uuid,
    2: double          tier_domain_uuid,
    3: bool            tier_domain_policy,
    4: i32             tier_media,
    5: i32             tier_prefetch_algo,
    6: i64            tier_media_pct,
    7: i64            tier_i32erval_sec,
}

struct FDSP_TierPolicyAudit {
    1: double          tier_vol_uuid,
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
		
   /* Message versioning for compatibility check, functionality changes*/
    2: i32        major_ver,  /* Major version number of this message */
    3: i32        minor_ver,  /* Minor version number of this message */
    4: i32        msg_id,     /* Message id to discard duplicate request & mai32ain causal order */

    5: i32        payload_len,
    6: i32        num_objects,  /* Payload could contain more than one object */
    7: i32        frag_len,     /* Fragment Length */
    8: i32        frag_num,     /* Fragment number for partial transfer */

/* Volume entity idenfiers */
    9: i32        tennant_id,      /* Tennant owning the Local-domain and Storage hypervisor */
    10: i32        local_domain_id, /* Local domain the volume in question bei64s */
    11: double        glob_volume_id,  /* Tennant owning the Local-domain and Storage hypervisor */
    12: string       bucket_name,    /* Bucket Name or Container Name for S3 or Azure entities */
		
    		/* Source and Destination Distributed s/w entities */
    13: FDSP_MgrIdType       src_id,
    14: FDSP_MgrIdType       dst_id,

    15: i64       		src_ip_hi_addr, /* IPv4 or IPv6 Address */
    16: i64       		src_ip_lo_addr, /* IPv4 or IPv6 Address */
    17: i64       		dst_ip_hi_addr, /* IPv4 or IPv6 address */
    18: i64       		dst_ip_lo_addr, /* IPv4 or IPv6 address */

    19: i32	src_port,
    20: i32 dst_port,
    21: string	src_node_name, /* string identifying the source node that sent this request */   

/* FDSP Result valid for response messages */
    22: FDSP_ResultType       result,
    23: string       	  err_msg,
    24: FDSP_ErrType          err_code,

/* Checksum of the entire message including the payload/objects */
    25: i32         req_cookie, 
    26: i32         msg_chksum, 
}

service FDSP_SessionReq {
    oneway void AssociateRespCallback(string src_node_name), // Associate Response callback ICE-object with DM/SM for this source node.
}

service FDSP_DataPathReq {
    oneway void GetObject(FDSP_MsgHdrType fdsp_msg, FDSP_GetObjType get_obj_req),

    oneway void PutObject(FDSP_MsgHdrType fdsp_msg, FDSP_PutObjType put_obj_req),

    oneway void DeleteObject(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteObjType del_obj_req),

    oneway void OffsetWriteObject(FDSP_MsgHdrType fdsp_msg, FDSP_OffsetWriteObjType offset_write_obj_req),

    oneway void RedirReadObject(FDSP_MsgHdrType fdsp_msg, FDSP_RedirReadObjType redir_write_obj_req),

}

service FDSP_MetaDataPathReq { 

    oneway void UpdateCatalogObject(FDSP_MsgHdrType fdsp_msg, FDSP_UpdateCatalogType cat_obj_req),

    oneway void QueryCatalogObject(FDSP_MsgHdrType fdsp_msg, FDSP_QueryCatalogType cat_obj_req),

    oneway void DeleteCatalogObject(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteCatalogType cat_obj_req),

    oneway void GetVolumeBlobList(FDSP_MsgHdrType fds_msg, FDSP_GetVolumeBlobListReqType blob_list_req),
}

service FDSP_MetaDataPathResp {

    oneway void UpdateCatalogObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_UpdateCatalogType cat_obj_req),

    oneway void QueryCatalogObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_QueryCatalogType cat_obj_req),

    oneway void DeleteCatalogObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteCatalogType cat_obj_req),

    oneway void GetVolumeBlobListResp(FDSP_MsgHdrType fds_msg, FDSP_GetVolumeBlobListRespType blob_list_rsp),
}

service FDSP_DataPathResp {
    oneway void GetObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_GetObjType get_obj_req),

    oneway void PutObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_PutObjType put_obj_req),

    oneway void DeleteObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteObjType del_obj_req),

    oneway void OffsetWriteObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_OffsetWriteObjType offset_write_obj_req),

    oneway void RedirReadObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_RedirReadObjType redir_write_obj_req),

}

service FDSP_ConfigPathReq {
  oneway i32 CreateVol(FDSP_MsgHdrType fdsp_msg, FDSP_CreateVolType crt_vol_req),
  oneway void DeleteVol(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteVolType del_vol_req),
  oneway void ModifyVol(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyVolType mod_vol_req),
  oneway void CreatePolicy(FDSP_MsgHdrType fdsp_msg, FDSP_CreatePolicyType crt_pol_req),
  oneway void DeletePolicy(FDSP_MsgHdrType fdsp_msg, FDSP_DeletePolicyType del_pol_req),
  oneway void ModifyPolicy(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyPolicyType mod_pol_req),
  oneway void AttachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType atc_vol_req),
  oneway void DetachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType dtc_vol_req),
  oneway void AssociateRespCallback(i64 ident), // Associate Response callback ICE-object with DM/SM 
  oneway i32 CreateDomain(FDSP_MsgHdrType fdsp_msg, FDSP_CreateDomainType crt_dom_req),
  oneway i32 DeleteDomain(FDSP_MsgHdrType fdsp_msg, FDSP_CreateDomainType del_dom_req),
  oneway void SetThrottleLevel(FDSP_MsgHdrType fdsp_msg, FDSP_ThrottleMsgType throttle_msg),	
  oneway void GetVolInfo(FDSP_MsgHdrType fdsp_msg, FDSP_GetVolInfoReqType vol_info_req),
  oneway void GetDomainStats(FDSP_MsgHdrType fdsp_msg, FDSP_GetDomainStatsType get_stats_msg),  

  /* 
  These are actually control messages from SM/DM/SH to OM. Need to move these to that control i32erface some time.
  For now, keeping it in config path i32erface since OM does not implement the control i32erface.
  */ 
  oneway void RegisterNode(FDSP_MsgHdrType fdsp_msg, FDSP_RegisterNodeType reg_node_req),
  oneway void NotifyQueueFull(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyQueueStateType queue_state_info),
  oneway void NotifyPerfstats(FDSP_MsgHdrType fdsp_msg, FDSP_PerfstatsType push_stats_msg),
  oneway void TestBucket(FDSP_MsgHdrType fdsp_msg, FDSP_TestBucket test_buck_msg),
}

service FDSP_ConfigPathResp {
  oneway void CreateVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_CreateVolType crt_vol_resp),
  oneway void DeleteVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteVolType del_vol_resp),
  oneway void ModifyVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyVolType mod_vol_resp),
  oneway void CreatePolicyResp(FDSP_MsgHdrType fdsp_msg, FDSP_CreatePolicyType crt_pol_resp),
  oneway void DeletePolicyResp(FDSP_MsgHdrType fdsp_msg, FDSP_DeletePolicyType del_pol_resp),
  oneway void ModifyPolicyResp(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyPolicyType mod_pol_resp),
  oneway void AttachVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType atc_vol_req),
  oneway void DetachVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType dtc_vol_req),
  oneway void RegisterNodeResp(FDSP_MsgHdrType fdsp_msg, FDSP_RegisterNodeType reg_node_resp),
  oneway void TestBucketResp(FDSP_MsgHdrType fdsp_msg, FDSP_TestBucket test_buck_resp),
  oneway i32 CreateDomainResp(FDSP_MsgHdrType fdsp_msg, FDSP_CreateDomainType crt_dom_resp),
  oneway i32 DeleteDomainResp(FDSP_MsgHdrType fdsp_msg, FDSP_CreateDomainType del_dom_resp),
  oneway void GetVolInfoResp(FDSP_MsgHdrType fdsp_msg, FDSP_GetVolInfoRespType vol_info_rsp),
  oneway void GetDomainStats(FDSP_MsgHdrType fdsp_msg, FDSP_GetDomainStatsType get_stats_rsp),
}

service FDSP_ControlPathReq {

  /* OM to SM/DM/SH control messages */

  oneway void NotifyAddVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_add_vol_req),
  oneway void NotifyRmVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_rm_vol_req),
  oneway void NotifyModVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_mod_vol_req),
  oneway void AttachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType atc_vol_req),
  oneway void DetachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType dtc_vol_req),
  oneway void NotifyNodeAdd(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info),
  oneway void NotifyNodeRmv(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info),
  oneway void NotifyDLTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Type dlt_info),
  oneway void NotifyDMTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DMT_Type dmt_info),
  oneway void SetThrottleLevel(FDSP_MsgHdrType fdsp_msg, FDSP_ThrottleMsgType throttle_msg),
  oneway void TierPolicy(FDSP_TierPolicy tier),
  oneway void TierPolicyAudit(FDSP_TierPolicyAudit audit),

 /* These should be config path response from OM to SH, but since SH does not implement the 
  * config resp interface, we keep those here for now
  */
  oneway void NotifyBucketStats(FDSP_MsgHdrType fdsp_msg, FDSP_BucketStatsRespType buck_stats_msg),
}

service FDSP_ControlPathResp {
  oneway void NotifyAddVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_add_vol_resp),
  oneway void NotifyRmVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_rm_vol_resp),
  oneway void NotifyModVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_mod_vol_resp),
  oneway void AttachVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType atc_vol_resp),
  oneway void DetachVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType dtc_vol_resp),
  oneway void NotifyNodeAddResp(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info_resp),
  oneway void NotifyNodeRmvResp(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info_resp),
  oneway void NotifyDLTUpdateResp(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Type dlt_info_resp),
  oneway void NotifyDMTUpdateResp(FDSP_MsgHdrType fdsp_msg, FDSP_DMT_Type dmt_info_resp),
}


#endif



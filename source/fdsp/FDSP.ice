#ifndef __FDSP_H__
#define __FDSP_H__
#include <Ice/Identity.ice>
#include <Ice/BuiltinSequences.ice>
#pragma once
module FDS_ProtocolInterface {


struct FDS_ObjectIdType {
  long   hash_high;
  long   hash_low;
  byte    conflict_id;
};

enum fds_dmgr_txn_state {
  FDS_DMGR_TXN_STATUS_INVALID,
  FDS_DMGR_TXN_STATUS_OPEN,
  FDS_DMGR_TXN_STATUS_COMMITED,
  FDS_DMGR_TXN_STATUS_CANCELED
};

enum FDSP_MsgCodeType {
   FDSP_MSG_PUT_OBJ_REQ,
   FDSP_MSG_GET_OBJ_REQ,
   FDSP_MSG_VERIFY_OBJ_REQ,
   FDSP_MSG_UPDATE_CAT_OBJ_REQ,
   FDSP_MSG_QUERY_CAT_OBJ_REQ,
   FDSP_MSG_GET_VOL_BLOB_LIST_REQ,
   FDSP_MSG_OFFSET_WRITE_OBJ_REQ,
   FDSP_MSG_REDIR_READ_OBJ_REQ,

   FDSP_MSG_PUT_OBJ_RSP,
   FDSP_MSG_GET_OBJ_RSP,
   FDSP_MSG_VERIFY_OBJ_RSP,
   FDSP_MSG_UPDATE_CAT_OBJ_RSP,
   FDSP_MSG_QUERY_CAT_OBJ_RSP,
   FDSP_MSG_GET_VOL_BLOB_LIST_RSP,
   FDSP_MSG_OFFSET_WRITE_OBJ_RSP,
   FDSP_MSG_REDIR_READ_OBJ_RSP,

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

};

enum FDSP_MgrIdType {
    FDSP_STOR_MGR,
    FDSP_DATA_MGR,
    FDSP_STOR_HVISOR,
    FDSP_ORCH_MGR,
    FDSP_CLI_MGR
};

enum FDSP_ResultType {
  FDSP_ERR_OK,
  FDSP_ERR_FAILED
};

enum FDSP_ErrType {
  FDSP_ERR_SM_NO_SPACE
};

enum FDSP_VolType {
  FDSP_VOL_S3_TYPE,
  FDSP_VOL_BLKDEV_TYPE,
  FDSP_VOL_BLKDEV_SSD_TYPE,
  FDSP_VOL_BLKDEV_DISK_TYPE,
  FDSP_VOL_BLKDEV_HYBRID_TYPE,
  FDSP_VOL_BLKDEV_HYBRID_PREFCAP_TYPE
};

enum FDSP_VolNotifyType {
  FDSP_NOTIFY_DEFAULT,
  FDSP_NOTIFY_ADD_VOL,
  FDSP_NOTIFY_RM_VOL,
  FDSP_NOTIFY_MOD_VOL
};

enum FDSP_ConsisProtoType {
  FDSP_CONS_PROTO_STRONG,
  FDSP_CONS_PROTO_WEAK,
  FDSP_CONS_PROTO_EVENTUAL
};

enum FDSP_AppWorkload {
    FDSP_APP_WKLD_TRANSACTION,
    FDSP_APP_WKLD_NOSQL,
    FDSP_APP_WKLD_HDFS,
    FDSP_APP_WKLD_JOURNAL_FILESYS,  // Ext3/ext4
    FDSP_APP_WKLD_FILESYS,  // XFS, other
    FDSP_APP_NATIVE_OBJS,  // Native object aka not going over http/rest apis
    FDSP_APP_S3_OBJS,  // Amazon S3 style objects workload
    FDSP_APP_AZURE_OBJS,  // Azure style objects workload
};

class FDSP_PutObjType {
  FDS_ObjectIdType   data_obj_id;
  int      data_obj_len;
  int      volume_offset; /* Offset inside the volume where the object resides */
  string data_obj;
};

class FDSP_GetObjType {
  FDS_ObjectIdType   data_obj_id;
  int      data_obj_len;
  string  data_obj;
};

class FDSP_OffsetWriteObjType {
  FDS_ObjectIdType   data_obj_id_old;
  int      data_obj_len;
  FDS_ObjectIdType   data_obj_id_new;
  string  data_obj;
};


class FDSP_RedirReadObjType {
  FDS_ObjectIdType   data_obj_id_old;
  int      data_obj_len;
  int      data_obj_suboffset; /* Offset within the object where the actual data is modified */
  int      data_obj_sublen;
  FDS_ObjectIdType   data_obj_id_new;
  string   data_obj;
}; 


class FDSP_VerifyObjType {
  FDS_ObjectIdType   data_obj_id;
  int      data_obj_len;
  string  data_obj;
};


class FDSP_UpdateCatalogType {
  int      volume_offset;       /* Offset inside the volume where the object resides */
  FDS_ObjectIdType   data_obj_id;         /* Object id of the object that this block is being mapped to */
  int      dm_transaction_id;  /* Transaction id */
  int      dm_operation;       /* Transaction type = OPEN, COMMIT, CANCEL */
};

class FDSP_QueryCatalogType {
  int      volume_offset;       /* Offset inside the volume where the object resides */
  FDS_ObjectIdType   data_obj_id;         /* Object id of the object that this block is being mapped to */
  int      dm_transaction_id;  /* Transaction id */
  int      dm_operation;       /* Transaction type = OPEN, COMMIT, CANCEL */
};

class FDSP_BlobInfoType{
  string blob_name;
};

sequence<FDSP_BlobInfoType> BlobInfoListType;

const long blob_list_iterator_begin = 0;
const long blob_list_iterator_end = 0xffffffff;

class FDSP_GetVolumeBlobListReqType {
  // take volUUID from the header
  int max_blobs_to_return; 
  long iterator_cookie; //set to "0" the first time and store and return for successive iterations
};

class FDSP_GetVolumeBlobListRespType {
  int num_blobs_in_resp;
  bool end_of_list; // true if there are no more blobs to return
  long iterator_cookie; // store and return this for the next iteration.
  BlobInfoListType blob_info_list; // list of blob_info structures.  
};


enum FDSP_NodeState {
     FDS_Node_Up,
     FDS_Node_Down,
     FDS_Node_Rmvd
};

class FDSP_Node_Info_Type {

  int      node_id;
  FDSP_NodeState     node_state;
  FDSP_MgrIdType node_type; /* Type of node - SM/DM/HV */
  string 	 node_name;    /* node indetifier - string */
  long		 ip_hi_addr; /* IP V6 high address */
  long		 ip_lo_addr; /* IP V4 address of V6 low address of the node */
  int		 control_port; /* Port number to contact for control messages */
  int		 data_port; /* Port number to send datapath requests */

};

sequence<FDSP_Node_Info_Type> Node_Info_List_Type;

sequence<int> Node_List_Type;
sequence<Node_List_Type> Node_Table_Type;


class FDSP_DLT_Type {
      int DLT_version;
      Node_Table_Type DLT;
};

class FDSP_DMT_Type {
      int DMT_version;
      Node_Table_Type DMT;
};


class FDSP_VolumeInfoType {

  string 		 vol_name;  /* Name of the volume or bucket*/
  int 	 		 tennantId;  // Tennant id that owns the volume
  int    		 localDomainId;  // Local domain id that owns vol
  int	 		 globDomainId;
  // double	 	 volUUID;

// Basic operational properties

  FDSP_VolType		 volType;		 		 
  double        	 capacity;
  double        	 maxQuota;  // Quota % of capacity tho should alert

// Consistency related properties

  int        		 defReplicaCnt;  // Number of replicas reqd for this volume
  int        		 defWriteQuorum;  // Quorum number of writes for success
  int        		 defReadQuorum;  // This will be 1 for now
  FDSP_ConsisProtoType 	 defConsisProtocol;  // Read-Write consistency protocol

// Other policies

  int        		 volPolicyId;
  int         		 archivePolicyId;
  int        		 placementPolicy;  // Can change placement policy
  FDSP_AppWorkload     	 appWorkload;

  int         		 backupVolume;  // UUID of backup volume

};

class FDSP_VolumeDescType {

  string 		 vol_name;  /* Name of the volume */
  int 	 		 tennantId;  // Tennant id that owns the volume
  int    		 localDomainId;  // Local domain id that owns vol
  int	 		 globDomainId;
  double	 	 volUUID;

// Basic operational properties

  FDSP_VolType		 volType;		 		 
  double        	 capacity;
  double        	 maxQuota;  // Quota % of capacity tho should alert

// Consistency related properties

  int        		 defReplicaCnt;  // Number of replicas reqd for this volume
  int        		 defWriteQuorum;  // Quorum number of writes for success
  int        		 defReadQuorum;  // This will be 1 for now
  FDSP_ConsisProtoType 	 defConsisProtocol;  // Read-Write consistency protocol

// Other policies

  int        		 volPolicyId;
  int         		 archivePolicyId;
  int        		 placementPolicy;  // Can change placement policy
  FDSP_AppWorkload     	 appWorkload;

  int         		 backupVolume;  // UUID of backup volume

// volume policy details 
  double                 iops_min; /* minimum (guaranteed) iops */
  double                 iops_max; /* maximum iops */
  int                    rel_prio; /* relative priority */
};

class FDSP_CreateDomainType {

  string 		 domain_name;
  int			 domain_id;

};

class FDSP_CreateVolType {

  string 		 vol_name;
  FDSP_VolumeInfoType 	 vol_info; /* Volume properties and attributes */

};

class FDSP_TestBucket {
  string 		 bucket_name;
  FDSP_VolumeInfoType 	 vol_info; /* Bucket properties and attributes */
  bool                   attach_vol_reqd; /* Should OMgr send out an attach volume if the bucket is accessible, etc */
  string                 accessKeyId;
  string                 secretAccessKey;
};

class FDSP_PolicyInfoType {
  string                 policy_name; /* Name of the policy */
  int                    policy_id;   /* policy id */
  double                 iops_min;    /* minimum (guaranteed) iops */
  double                 iops_max;    /* maximum iops */
  int                    rel_prio;    /* relative priority */
};

class FDSP_DeleteVolType {
  string 		 vol_name;  /* Name of the volume */
  // double    		 vol_uuid;
  int			 domain_id;
};

class FDSP_ModifyVolType {
  string 		 vol_name;  /* Name of the volume */
  double		 vol_uuid;
  FDSP_VolumeInfoType	 vol_info;  /* New updated volume properties */
};

class FDSP_AttachVolCmdType {
  string		 vol_name; // Name of the volume to attach
  // double		 vol_uuid; // UUID of the volume being attached
  string		 node_id;  // Id of the hypervisor node where the volume should be attached
  int			 domain_id;
};

class FDSP_NotifyVolType {
  FDSP_VolNotifyType 	 type;       /* Type of notify */
  string             	 vol_name;   /* Name of the volume */
  FDSP_VolumeDescType	 vol_desc;   /* Volume properties and attributes */
};

class FDSP_AttachVolType {
  string 		 vol_name; /* Name of the volume */
  FDSP_VolumeDescType	 vol_desc; /* Volume properties and attributes */
};

class FDSP_CreatePolicyType {
  string                 policy_name;  /* Name of the policy */
  FDSP_PolicyInfoType 	 policy_info;  /* Policy description */
};

class FDSP_DeletePolicyType {
  string                 policy_name;  /* Name of the policy */
  int                    policy_id;    /* policy id */
};

class FDSP_ModifyPolicyType {
  string                 policy_name;  /* Name of the policy */
  int                    policy_id;    /* policy id */
  FDSP_PolicyInfoType 	 policy_info;  /* Policy description */
};

class FDSP_AnnounceDiskCapability {
  int	disk_iops_max;  /* iops suppported by */
  int	disk_iops_min;  /* iops suppported by */
  double	disk_capacity; /* size of the disk  */
  int	disk_latency_max;  /* latency  */
  int	disk_latency_min;  /* latency  */
  int 	ssd_iops_max;  /* iops suppported by */
  int 	ssd_iops_min;  /* iops suppported by */
  double	ssd_capacity; /* size of the disk  */
  int	ssd_latency_max;  /* latency  */
  int	ssd_latency_min;  /* latency  */
  int	disk_type;  /* disk type  */
};

class FDSP_RegisterNodeType {
  FDSP_MgrIdType node_type; /* Type of node - SM/DM/HV */
  string 	 node_name;    /* node indetifier - string */
  int 	     domain_id;    /* domain indetifier */
  long		 ip_hi_addr; /* IP V6 high address */
  long		 ip_lo_addr; /* IP V4 address of V6 low address of the node */
  int		 control_port; /* Port number to contact for control messages */
  int		 data_port; /* Port number to send datapath requests */
  FDSP_AnnounceDiskCapability  disk_info; /* Add node capacity and other relevant fields here */
};

class FDSP_ThrottleMsgType {
  int 	         domain_id; /* Domain this throttle message is meant for */
  float	         throttle_level; /* a real number between -10 and 10 */
  /* 
     A throttle level of X.Y (e.g, 5.6) means we should
     1. throttle all traffic for priorities greater than X (priorities 6,7,8,9 for a 5.6 throttle level) to their guaranteed min rate,
     2. allow all traffic for priorities less than X (priorities 0,1,2,3,4 for a 5.6 throttle level) to go up till their max rate limit,
     3. throttle traffic for priority X to a rate = min_rate + Y/10 * (max_rate - min_rate). 
        (A volume that has a min rate of 300 IOPS and max rate of 600 IOPS will be allowed to pump at 480 IOPS when throttle level is 5.6).
     
     A throttle level of 0 indicates all volumes should be limited at their min_iops rating.
     A negative throttle level of -X means all volumes should be throttled at (10-X)/10 of their min iops. 
  */
};

class FDSP_PerfStatType {
  long   nios;     /* number of IOs in stat time interval  */
  long   min_lat;  /* minimum latency */
  long   max_lat;  /* maximum latency */
  double ave_lat;  /* average latency */

  int stat_type;     /* 0 - read/disk, 1 - write/disk, 2 - read/flash, 3 - write/flash, 5 - total 
                      * Note that SH will only return stat_type 5 (for now) */
  long rel_seconds;  /* timestamp -- in seconds relative to FDSP_PerfstatsType::start_timestamp */
};

sequence<FDSP_PerfStatType> FDSP_PerfStatListType;
 
class FDSP_VolPerfHistType {
  double vol_uuid;
  FDSP_PerfStatListType  stat_list;  /* list of performance stats (one or more time slots) for this volume */
};

sequence<FDSP_VolPerfHistType> FDSP_VolPerfHistListType;

class FDSP_PerfstatsType {
  FDSP_MgrIdType            node_type; /* type of node - SM/SH */ 
  int                       slot_len_sec; /* length of each stat time slot */
  string                    start_timestamp; /* to calc absolute timestamps of stats which contain relative timestamps */
  FDSP_VolPerfHistListType  vol_hist_list; /* list of performance histories of volumes */
};

class FDSP_QueueStateType {

  int domain_id;
  double vol_uuid;
  int priority;
  float queue_depth; //current queue depth as a fraction of the total queue size. 0.5 means 50% full.
  
};

class FDSP_TierPolicy {
    double          tier_vol_uuid;
    double          tier_domain_uuid;
    bool            tier_domain_policy;
    int             tier_media;
    int             tier_prefetch_algo;
    long            tier_media_pct;
    long            tier_interval_sec;
};

class FDSP_TierPolicyAudit {
    double          tier_vol_uuid;
    long            tier_stat_min_iops;
    long            tier_stat_max_iops;
    long            tier_pct_ssd_iop;
    long            tier_pct_hdd_iop;
    long            tier_pct_ssd_capacity;
};

sequence<FDSP_QueueStateType> FDSP_QueueStateListType;

class FDSP_NotifyQueueStateType {

  FDSP_QueueStateListType queue_state_list; 

};

class FDSP_MsgHdrType {
    FDSP_MsgCodeType     msg_code;
		
   /* Message versioning for compatibility check, functionality changes*/
    int        major_ver;  /* Major version number of this message */
    int        minor_ver;  /* Minor version number of this message */
    int        msg_id;     /* Message id to discard duplicate request & maintain causal order */

    int        payload_len;
    int        num_objects;  /* Payload could contain more than one object */
    int        frag_len;     /* Fragment Length */
    int        frag_num;     /* Fragment number for partial transfer */

/* Volume entity idenfiers */
    int        tennant_id;      /* Tennant owning the Local-domain and Storage hypervisor */
    int        local_domain_id; /* Local domain the volume in question belongs */
    double        glob_volume_id;  /* Tennant owning the Local-domain and Storage hypervisor */
    string       bucket_name;    /* Bucket Name or Container Name for S3 or Azure entities */
		
    		/* Source and Destination Distributed s/w entities */
    FDSP_MgrIdType       src_id;
    FDSP_MgrIdType       dst_id;

    long       		src_ip_hi_addr; /* IPv4 or IPv6 Address */
    long       		src_ip_lo_addr; /* IPv4 or IPv6 Address */
    long       		dst_ip_hi_addr; /* IPv4 or IPv6 address */
    long       		dst_ip_lo_addr; /* IPv4 or IPv6 address */

    int	src_port;
    int dst_port;
    string	src_node_name; /* string identifying the source node that sent this request */   

/* FDSP Result valid for response messages */
    FDSP_ResultType       result;
    string       	  err_msg;
    FDSP_ErrType          err_code;

/* Checksum of the entire message including the payload/objects */
    int         req_cookie; 
    int         msg_chksum; 
};

interface FDSP_DataPathReq {
    void GetObject(FDSP_MsgHdrType fdsp_msg, FDSP_GetObjType get_obj_req);

    void PutObject(FDSP_MsgHdrType fdsp_msg, FDSP_PutObjType put_obj_req);

    void UpdateCatalogObject(FDSP_MsgHdrType fdsp_msg, FDSP_UpdateCatalogType cat_obj_req);

    void QueryCatalogObject(FDSP_MsgHdrType fdsp_msg, FDSP_QueryCatalogType cat_obj_req);

    void OffsetWriteObject(FDSP_MsgHdrType fdsp_msg, FDSP_OffsetWriteObjType offset_write_obj_req);

    void RedirReadObject(FDSP_MsgHdrType fdsp_msg, FDSP_RedirReadObjType redir_write_obj_req);

    void AssociateRespCallback(Ice::Identity ident, string src_node_name); // Associate Response callback ICE-object with DM/SM for this source node.

    void GetVolumeBlobList(FDSP_MsgHdrType fds_msg, FDSP_GetVolumeBlobListReqType blob_list_req);

};

interface FDSP_DataPathResp {
    void GetObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_GetObjType get_obj_req);

    void PutObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_PutObjType put_obj_req);

    void UpdateCatalogObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_UpdateCatalogType cat_obj_req);

    void QueryCatalogObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_QueryCatalogType cat_obj_req);

    void OffsetWriteObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_OffsetWriteObjType offset_write_obj_req);

    void RedirReadObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_RedirReadObjType redir_write_obj_req);

    void GetVolumeBlobListResp(FDSP_MsgHdrType fds_msg, FDSP_GetVolumeBlobListRespType blob_list_rsp);

};

interface FDSP_ConfigPathReq {
  int CreateVol(FDSP_MsgHdrType fdsp_msg, FDSP_CreateVolType crt_vol_req);
  void DeleteVol(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteVolType del_vol_req);
  void ModifyVol(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyVolType mod_vol_req);
  void CreatePolicy(FDSP_MsgHdrType fdsp_msg, FDSP_CreatePolicyType crt_pol_req);
  void DeletePolicy(FDSP_MsgHdrType fdsp_msg, FDSP_DeletePolicyType del_pol_req);
  void ModifyPolicy(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyPolicyType mod_pol_req);
  void AttachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType atc_vol_req);
  void DetachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType dtc_vol_req);
  void AssociateRespCallback(Ice::Identity ident); // Associate Response callback ICE-object with DM/SM 
  int CreateDomain(FDSP_MsgHdrType fdsp_msg, FDSP_CreateDomainType crt_dom_req);
  int DeleteDomain(FDSP_MsgHdrType fdsp_msg, FDSP_CreateDomainType del_dom_req);
  void SetThrottleLevel(FDSP_MsgHdrType fdsp_msg, FDSP_ThrottleMsgType throttle_msg);	

  /* 
  These are actually control messages from SM/DM/SH to OM. Need to move these to that control interface some time.
  For now, keeping it in config path interface since OM does not implement the control interface.
  */ 
  void RegisterNode(FDSP_MsgHdrType fdsp_msg, FDSP_RegisterNodeType reg_node_req);
  void NotifyQueueFull(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyQueueStateType queue_state_info);
  void NotifyPerfstats(FDSP_MsgHdrType fdsp_msg, FDSP_PerfstatsType push_stats_msg);
  void TestBucket(FDSP_MsgHdrType fdsp_msg, FDSP_TestBucket test_buck_msg);
};

interface FDSP_ConfigPathResp {
  void CreateVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_CreateVolType crt_vol_resp);
  void DeleteVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteVolType del_vol_resp);
  void ModifyVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyVolType mod_vol_resp);
  void CreatePolicyResp(FDSP_MsgHdrType fdsp_msg, FDSP_CreatePolicyType crt_pol_resp);
  void DeletePolicyResp(FDSP_MsgHdrType fdsp_msg, FDSP_DeletePolicyType del_pol_resp);
  void ModifyPolicyResp(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyPolicyType mod_pol_resp);
  void AttachVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType atc_vol_req);
  void DetachVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType dtc_vol_req);
  void RegisterNodeResp(FDSP_MsgHdrType fdsp_msg, FDSP_RegisterNodeType reg_node_resp);
  void TestBucketResp(FDSP_MsgHdrType fdsp_msg, FDSP_TestBucket test_buck_resp);
  int CreateDomainResp(FDSP_MsgHdrType fdsp_msg, FDSP_CreateDomainType crt_dom_resp);
  int DeleteDomainResp(FDSP_MsgHdrType fdsp_msg, FDSP_CreateDomainType del_dom_resp);
};

interface FDSP_ControlPathReq {

  /* OM to SM/DM/SH control messages */

  void NotifyAddVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_add_vol_req);
  void NotifyRmVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_rm_vol_req);
  void AttachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType atc_vol_req);
  void DetachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType dtc_vol_req);
  void NotifyNodeAdd(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info);
  void NotifyNodeRmv(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info);
  void NotifyDLTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Type dlt_info);
  void NotifyDMTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DMT_Type dmt_info);
  void SetThrottleLevel(FDSP_MsgHdrType fdsp_msg, FDSP_ThrottleMsgType throttle_msg);
  void TierPolicy(FDSP_TierPolicy tier);
  void TierPolicyAudit(FDSP_TierPolicyAudit audit);

};

interface FDSP_ControlPathResp {
  void NotifyAddVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_add_vol_resp);
  void NotifyRmVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_rm_vol_resp);
  void AttachVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType atc_vol_resp);
  void DetachVolResp(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType dtc_vol_resp);
  void NotifyNodeAddResp(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info_resp);
  void NotifyNodeRmvResp(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info_resp);
  void NotifyDLTUpdateResp(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Type dlt_info_resp);
  void NotifyDMTUpdateResp(FDSP_MsgHdrType fdsp_msg, FDSP_DMT_Type dmt_info_resp);

};

};
#endif



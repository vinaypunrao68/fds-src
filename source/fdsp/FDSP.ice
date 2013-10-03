#ifndef __FDSP_H__
#define __FDSP_H__
#include <Ice/Identity.ice>
#include <Ice/BuiltinSequences.ice>
#pragma once
module FDS_ProtocolInterface {


struct FDS_ObjectIdType {
  double   hash_high;
  double   hash_low;
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
   FDSP_MSG_OFFSET_WRITE_OBJ_REQ,
   FDSP_MSG_REDIR_READ_OBJ_REQ,

   FDSP_MSG_PUT_OBJ_RSP,
   FDSP_MSG_GET_OBJ_RSP,
   FDSP_MSG_VERIFY_OBJ_RSP,
   FDSP_MSG_UPDATE_CAT_OBJ_RSP,
   FDSP_MSG_QUERY_CAT_OBJ_RSP,
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
  FDSP_VOL_BLKDEV_SSD_TYPE
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

  int        		 replicaCnt;  // Number of replicas reqd for this volume
  int        		 writeQuorum;  // Quorum number of writes for success
  int        		 readQuorum;  // This will be 1 for now
  FDSP_ConsisProtoType 	 consisProtocol;  // Read-Write consistency protocol

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

  int        		 replicaCnt;  // Number of replicas reqd for this volume
  int        		 writeQuorum;  // Quorum number of writes for success
  int        		 readQuorum;  // This will be 1 for now
  FDSP_ConsisProtoType 	 consisProtocol;  // Read-Write consistency protocol

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

class FDSP_CreateVolType {

  string 		 vol_name;
  FDSP_VolumeInfoType 	 vol_info; /* Volume properties and attributes */

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
  double    		 vol_uuid;
};

class FDSP_ModifyVolType {
  string 		 vol_name;  /* Name of the volume */
  double		 vol_uuid;
  FDSP_VolumeInfoType	 vol_info;  /* New updated volume properties */
};

class FDSP_AttachVolCmdType {
  string		 vol_name; // Name of the volume to attach
  double		 vol_uuid; // UUID of the volume being attached
  string		 node_id;  // Id of the hypervisor node where the volume should be attached
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

class FDSP_RegisterNodeType {
  FDSP_MgrIdType node_type; /* Type of node - SM/DM/HV */
  string 	 node_name;    /* node indetifier - string */
  long		 ip_hi_addr; /* IP V6 high address */
  long		 ip_lo_addr; /* IP V4 address of V6 low address of the node */
  int		 control_port; /* Port number to contact for control messages */
  int		 data_port; /* Port number to send datapath requests */
  /* Add node capacity and other relevant fields here */
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
		
    		/* Source and Destination Distributed s/w entities */
    FDSP_MgrIdType       src_id;
    FDSP_MgrIdType       dst_id;

    long       		src_ip_hi_addr; /* IPv4 or IPv6 Address */
    long       		src_ip_lo_addr; /* IPv4 or IPv6 Address */
    long       		dst_ip_hi_addr; /* IPv4 or IPv6 address */
    long       		dst_ip_lo_addr; /* IPv4 or IPv6 address */

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

    void AssociateRespCallback(Ice::Identity ident); // Associate Response callback ICE-object with DM/SM 
};

interface FDSP_DataPathResp {
    void GetObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_GetObjType get_obj_req);

    void PutObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_PutObjType put_obj_req);

    void UpdateCatalogObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_UpdateCatalogType cat_obj_req);

    void QueryCatalogObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_QueryCatalogType cat_obj_req);

    void OffsetWriteObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_OffsetWriteObjType offset_write_obj_req);

    void RedirReadObjectResp(FDSP_MsgHdrType fdsp_msg, FDSP_RedirReadObjType redir_write_obj_req);

};

interface FDSP_ConfigPathReq {
  void CreateVol(FDSP_MsgHdrType fdsp_msg, FDSP_CreateVolType crt_vol_req);
  void DeleteVol(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteVolType del_vol_req);
  void ModifyVol(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyVolType mod_vol_req);
  void CreatePolicy(FDSP_MsgHdrType fdsp_msg, FDSP_CreatePolicyType crt_pol_req);
  void DeletePolicy(FDSP_MsgHdrType fdsp_msg, FDSP_DeletePolicyType del_pol_req);
  void ModifyPolicy(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyPolicyType mod_pol_req);
  void AttachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType atc_vol_req);
  void DetachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType dtc_vol_req);
  void RegisterNode(FDSP_MsgHdrType fdsp_msg, FDSP_RegisterNodeType reg_node_req);
  void AssociateRespCallback(Ice::Identity ident); // Associate Response callback ICE-object with DM/SM 
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
};

interface FDSP_ControlPathReq {
  void NotifyAddVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_add_vol_req);
  void NotifyRmVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_rm_vol_req);
  void AttachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType atc_vol_req);
  void DetachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType dtc_vol_req);
  void NotifyNodeAdd(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info);
  void NotifyNodeRmv(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info);
  void NotifyDLTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Type dlt_info);
  void NotifyDMTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DMT_Type dmt_info);
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



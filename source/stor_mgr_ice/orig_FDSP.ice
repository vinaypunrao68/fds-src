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
   FDSP_MSG_OFFSET_WRITE_OBJ_REQ,
   FDSP_MSG_REDIR_READ_OBJ_REQ,

   FDSP_MSG_PUT_OBJ_RSP,
   FDSP_MSG_GET_OBJ_RSP,
   FDSP_MSG_VERIFY_OBJ_RSP,
   FDSP_MSG_UPDATE_CAT_OBJ_RSP,
   FDSP_MSG_OFFSET_WRITE_OBJ_RSP,
   FDSP_MSG_REDIR_READ_OBJ_RSP
};

enum FDSP_MgrIdType {
    FDSP_STOR_MGR,
    FDSP_DATA_MGR,
    FDSP_STOR_HVISOR,
    FDSP_ORCH_MGR
};

enum FDSP_ResultType {
  FDSP_ERR_OK,
  FDSP_ERR_FAILED
};

enum FDSP_ErrType {
  FDSP_ERR_SM_NO_SPACE
};


class FDSP_PutObjType {
  FDS_ObjectIdType   data_obj_id;
  int      data_obj_len;
  int      volume_offset; /* Offset inside the volume where the object resides */
  Ice::ByteSeq data_obj;
};

class FDSP_GetObjType {
  FDS_ObjectIdType   data_obj_id;
  int      data_obj_len;
  Ice::ByteSeq   data_obj;
};

class FDSP_OffsetWriteObjType {
  FDS_ObjectIdType   data_obj_id_old;
  int      data_obj_len;
  FDS_ObjectIdType   data_obj_id_new;
  Ice::ByteSeq  data_obj;
};


class FDSP_RedirReadObjType {
  FDS_ObjectIdType   data_obj_id_old;
  int      data_obj_len;
  int      data_obj_suboffset; /* Offset within the object where the actual data is modified */
  int      data_obj_sublen;
  FDS_ObjectIdType   data_obj_id_new;
  Ice::ByteSeq   data_obj;
}; 


class FDSP_VerifyObjType {
  FDS_ObjectIdType   data_obj_id;
  int      data_obj_len;
  Ice::ByteSeq  data_obj;
};


class FDSP_UpdateCatalogType {
  int      volume_offset;       /* Offset inside the volume where the object resides */
  FDS_ObjectIdType   data_obj_id;         /* Object id of the object that this block is being mapped to */
  int      dm_transaction_id;  /* Transaction id */
  int      dm_operation;       /* Transaction type = OPEN, COMMIT, CANCEL */
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
    int        glob_volume_id;  /* Tennant owning the Local-domain and Storage hypervisor */
		
    		/* Source and Destination Distributed s/w entities */
    FDSP_MgrIdType       src_id;
    FDSP_MgrIdType       dst_id;

    long       		src_ip_hi_addr; /* IPv4 or IPv6 Address */
    long       		src_ip_lo_addr; /* IPv4 or IPv6 Address */
    long       		dst_ip_hi_addr; /* IPv4 or IPv6 address */
    long       		dst_ip_lo_addr; /* IPv4 or IPv6 address */

/* FDSP Result valid for response messages */
    FDSP_ResultType       result;
    string       err_msg;
    FDSP_ErrType          err_code;

/* Checksum of the entire message including the payload/objects */
    int         req_cookie; 
    int         msg_chksum; 
		
};

interface FDSP_DataPathReq {
    void GetObject(FDSP_MsgHdrType *fdsp_msg, FDSP_GetObjType *get_obj_req);

    void PutObject(FDSP_MsgHdrType *fdsp_msg, FDSP_PutObjType *put_obj_req);

    void UpdateCatalogObject(FDSP_MsgHdrType *fdsp_msg, FDSP_UpdateCatalogType *cat_obj_req);

    void OffsetWriteObject(FDSP_MsgHdrType *fdsp_msg, FDSP_OffsetWriteObjType *offset_write_obj_req);

    void RedirReadObject(FDSP_MsgHdrType *fdsp_msg, FDSP_RedirReadObjType *redir_write_obj_req);
};

interface FDSP_DataPathRespCback {
    void GetObjectResp(FDSP_MsgHdrType *fdsp_msg, FDSP_GetObjType *get_obj_req);

    void PutObjectResp(FDSP_MsgHdrType *fdsp_msg, FDSP_PutObjType *put_obj_req);

    void UpdateCatalogObjectResp(FDSP_MsgHdrType *fdsp_msg, FDSP_UpdateCatalogType *cat_obj_req);

    void OffsetWriteObjectResp(FDSP_MsgHdrType *fdsp_msg, FDSP_OffsetWriteObjType *offset_write_obj_req);

    void RedirReadObjectResp(FDSP_MsgHdrType *fdsp_msg, FDSP_RedirReadObjType *redir_write_obj_req);

};

};
#endif



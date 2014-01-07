
namespace cpp FDS_ProtocolInterface

struct FDS_ObjectIdType {
  1: i64   hash_high,
  2: i64   hash_low,
  3: byte    conflict_id
}


enum FDSP_MsgCodeType {
   FDSP_MSG_PUT_OBJ_REQ,
   FDSP_MSG_GET_OBJ_REQ,
   FDSP_MSG_DELETE_OBJ_REQ
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
  1: FDS_ObjectIdType   data_obj_idr,
  2: i32      data_obj_len
}

struct  FDSP_MsgHdrType {
    1: FDSP_MsgCodeType     msg_code,

   /* Message versioning for compatibility check, functionality changes*/
    2: i32        major_ve,  /* Major version number of this message */
    3: i32        minor_ver,  /* Minor version number of this message */
    4: i32        msg_id,     /* Message id to discard duplicate request & maintain causal order */

    5: i32        payload_len,
    6: i32        num_objects,  /* Payload could contain more than one object */
    7: i32        frag_len,     /* Fragment Length */
    8:  i32        frag_num,     /* Fragment number for partial transfer */

/* Volume entity idenfiers */
    9: i32        tennant_id,      /* Tennant owning the Local-domain and Storage hypervisor */
    10: i32        local_domain_id, /* Local domain the volume in question belongs */
    11: double        glob_volume_id,  /* Tennant owning the Local-domain and Storage hypervisor */
    12: string       bucket_name,    /* Bucket Name or Container Name for S3 or Azure entities */

                /* Source and Destination Distributed s/w entities */
    13: FDSP_MgrIdType       src_id,
    14: FDSP_MgrIdType       dst_id,

    15: i64                src_ip_hi_addr, /* IPv4 or IPv6 Address */
    16: i64                src_ip_lo_addr, /* IPv4 or IPv6 Address */
    17: i64                dst_ip_hi_addr, /* IPv4 or IPv6 address */
    18: i64                dst_ip_lo_addr, /* IPv4 or IPv6 address */

    19: i32 src_port,
    20:i32 dst_port,
    21: string      src_node_name, /* string identifying the source node that sent this request */

/* FDSP Result valid for response messages */
    22: FDSP_ResultType       result,
    23: string                err_msg,
    24: FDSP_ErrType          err_code,

/* Checksum of the entire message including the payload/objects */
    25: i32         req_cookie,
    26: i32         msg_chksum
}



service  FDSP_DataPathReq {
    oneway void GetObject(1: FDSP_MsgHdrType fdsp_msg, 2: FDSP_GetObjType get_obj_req),

    oneway void PutObject(1: FDSP_MsgHdrType fdsp_msg, 2: FDSP_PutObjType put_obj_req),

    oneway void DeleteObject(1: FDSP_MsgHdrType fdsp_msg, 2: FDSP_DeleteObjType del_obj_req),

    oneway void AssociateRespCallback(1:string src_node_name)

}

service  FDSP_DataPathResp {
    oneway void GetObjectResp(1: FDSP_MsgHdrType fdsp_msg, 2: FDSP_GetObjType get_obj_req),

    oneway void PutObjectResp(1: FDSP_MsgHdrType fdsp_msg, 2: FDSP_PutObjType put_obj_req),

    oneway void DeleteObjectResp(1: FDSP_MsgHdrType fdsp_msg, 2: FDSP_DeleteObjType del_obj_req)
}

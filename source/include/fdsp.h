#include <fds_commons.h>

typedef enum {
   FDSP_MSG_PUT_OBJ_REQ,
   FDSP_MSG_GET_OBJ_REQ,
   FDSP_MSG_VERIFY_OBJ_REQ,
   FDSP_MSG_UPDATE_CAT_OBJ_REQ,
   FDSP_MSG_OFFSET_WRITE_OBJ_REQ,

   FDSP_MSG_PUT_OBJ_RSP,
   FDSP_MSG_GET_OBJ_RSP,
   FDSP_MSG_VERIFY_OBJ_RSP,
   FDSP_MSG_UPDATE_CAT_OBJ_RSP
   FDSP_MSG_OFFSET_WRITE_OBJ_RSP,
};

typedef struct _fdsp_put_object_t {
  fds_object_id_t   data_obj_id;
  fds_uint32_t      data_obj_len;
  fds_uint32_t      volume_offset; /* Offset inside the volume where the object resides */
  fds_char *        data_obj;
} fdsp_put_object_t;

typedef struct _fdsp_get_object_t {
  fds_object_id_t   data_obj_id;
  fds_uint32_t      data_obj_len;
  fds_char *        data_obj;
} fdsp_get_object_t;

typedef struct _fdsp_offset_write_object_t {
  fds_object_id_t   data_obj_id_old;
  fds_uint32_t      data_obj_len;
  fds_object_id_t   data_obj_id_new;
  fds_char *        data_obj;
} fdsp_offset_write_object_t;

typedef struct _fdsp_verify_object_t {
  fds_object_id_t   data_obj_id;
  fds_uint32_t      data_obj_len;
  fds_char *        data_obj;
} fdsp_verify_object_t;

typedef struct _fdsp_update_catalog_t {
  fds_uint32_t      volume_offset;       /* Offset inside the volume where the object resides */
  fds_uint32_t      dm_transaction_id;  /* Transaction id */
  fds_uint32_t      dm_operation;       /* Transaction type = OPEN, COMMIT, CANCEL */
} fdsp_update_catalog_t;

typedef union {
    fdsp_put_object_t     put_obj;
    fdsp_get_object_t     get_obj;
    fdsp_verify_object_t  verify_obj;
    fdsp_update_catalog_t update_catalog;
    fdsp_offset_write_t   offset_write;
} fdsp_payload_t;


typedef enum {
    FDSP_STOR_MGR,
    FDSP_DATA_MGR,
    FDSP_STOR_HVISOR,
    FDSP_ORCH_MGR
} fdsp_mgr_id_t;

typedef _fdsp_msg_t {
    fdsp_msg_code_t     msg_code;

    /* Message versioning for compatibility check, functionality changes*/
    fds_uint32_t        major_ver;  /* Major version number of this message */
    fds_uint32_t        minor_ver;  /* Minor version number of this message */
    fds_uint32_t        msg_id;     /* Message id to discard duplicate request & maintain causal order */

    fds_uint32_t        payload_len;
    fds_uint32_t        num_objects;  /* Payload could contain more than one object */
    fds_uint32_t        frag_len;     /* Fragment Length */

    /* Volume entity idenfiers */
    fds_uint32_t        tennant_id;      /* Tennant owning the Local-domain and Storage hypervisor */
    fds_uint32_t        local_domain_id; /* Local domain the volume in question belongs */
    fds_uint32_t        glob_volume_id;  /* Tennant owning the Local-domain and Storage hypervisor */

    /* Source and Destination Distributed s/w entities */
    fdsp_mgr_id_t       src_id;
    fdsp_mgr_id_t       dst_id;
    fds_uint32_t        src_ip_addr[4]; /* IPv4 or IPv6 Address */
    fds_uint32_t        dst_ip_addr[4]; /* IPv4 or IPv6 address */

    /* FDSP Result valid for response messages */
    fdsp_result_t       result;
    fdsp_error_msg_t    err_msg;
    fdsp_err_t          err_code;

    /* Checksum of the entire message including the payload/objects */
    fds_int32_t         msg_chksum; 

    /* FDSP actual payload, variable length */
    fdsp_payload_t      payload;
} fdsp_msg_t;

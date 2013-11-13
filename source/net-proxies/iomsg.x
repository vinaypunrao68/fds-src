/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * IOMesg RPC IDL used in simple test hook probe that enable us to connect any
 * front-end IO load/pattern generator to any layer in the FDS's back-end.
 *
 * Structures to put/get objects in data path and control them in admin path.
 */
struct obj_io_hdr_t
{
    double                   io_oid_hi;
    double                   io_oid_lo;
    double                   io_vol_uuid;
    double                   io_blk_off;   /* volume offset in blocks unit. */
    unsigned int             io_len_blk;   /* request len in blocks unit. */
};

struct obj_io_req_t
{
    obj_io_hdr_t             io_hdr;
    opaque                   io_data<>;
};

/*
 * Standard response in the data path.
 */
struct obj_io_resp_t
{
    obj_io_hdr_t            io_hdr;
    int                     io_status;
    opaque                  io_resp<>;
};

/*
 * Request in admin paths.  Hook up with FDS's cli to create a "volume"
 * device for the test's front-end probe.
 */
const PROBE_MAX_VOL_NAME = 32;

struct vol_create_t
{
    double                   vol_uuid;
    double                   vol_size_blk;
    char                     vol_name[PROBE_MAX_VOL_NAME];
};

struct vol_delete_t
{
    double                   vol_uuid;
    char                     vol_name[PROBE_MAX_VOL_NAME];
};

/*
 * Standard response in the control/admin path.
 */
struct obj_ctrl_resp_t
{
    double                  ct_vol_uuid;
    int                     ct_status;
    opaque                  ct_text<>;
};

program                  OBJECT_SERVER {
    version                OBJ_SRV_VERS {
        obj_io_resp_t        OBJ_GET(obj_io_req_t) = 1;
        obj_io_resp_t        OBJ_PUT(obj_io_req_t) = 2;
    } = 1;
} = 0x20000002;

program                  OBJECT_SERVER_ADMIN {
    version                OBJ_SRV_ADMIN_VERS {
        obj_ctrl_resp_t      OBJ_VOL_CREATE(vol_create_t) = 1;
        obj_ctrl_resp_t      OBJ_VOL_DELETE(vol_delete_t) = 2;
    } = 1;
} = 0x20000001;

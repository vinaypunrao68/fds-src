/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
namespace cpp fpi

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

/*
 * This message header is owned, controlled and set by the net service layer.
 * Application code treats it as opaque type.
 */
struct AsyncHdr {
    1: required i32           msg_chksum,
    2: required i32           msg_src_id,
    3: required SvcUuid       msg_src_uuid,
    4: required SvcUuid       msg_dst_uuid,
}

struct AsyncRspHdr {
    1: required AsyncHdr      msg_resp,
    2: required i32           msg_status,
    3: optional string        msg_text,
    4: optional string        msg_data
}


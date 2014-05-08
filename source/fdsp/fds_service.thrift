/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
namespace cpp FDS_ProtocolInterface

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

struct DomainID {
    1: required SvcUuid       domain_id,
    2: required string        domain_name;
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

struct GetObjReq {
    1: required AsyncHdr      header;
    2: required string        id;
}
struct PutObjReq {
    1: required AsyncHdr      header;
    2: required string        id;
    3: required string        data;
}

struct GetObjRsp {
	1: required i32       len;
	2: required string    data;
}

struct PutObjRsp {
	1: required i32       status;
}

service AsyncRspSvc {
	oneway void asyncResponse(1: AsyncHdr header, 2: string payload)
}

service SMSvc extends AsyncRspSvc {
	oneway void getObject(1: GetObjReq req)
	oneway void putObject(1: PutObjReq req)
}

service AMSvc extends AsyncRspSvc {
}

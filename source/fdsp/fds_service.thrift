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
    2: required string        domain_name
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
    5: required i32           msg_code,
}

struct GetObjReq {
    1: required AsyncHdr      header,
    2: required string        id
}
struct PutObjReq {
    1: required AsyncHdr      header,
    2: required string        id,
    3: required string        data,
}

struct GetObjRsp {
    1: required i32       len;
    2: required string    data;
}

struct PutObjRsp {
    1: required i32       status;
}

/*
 * Uuid to physical location binding registration.
 */
struct UuidBindMsg {
    1: required AsyncHdr     header,
    2: required SvcID        svc_id,
    3: required string       svc_addr,
    4: required i32          scv_port
}

service BaseAsyncSvc {
    oneway void asyncReqt(1: AsyncHdr header),
    oneway void asyncResp(1: AsyncHdr header, 2: string payload),
    oneway void uuidBind(1: UuidBindMsg msg)
}

service SMSvc extends BaseAsyncSvc {
    oneway void getObject(1: GetObjReq req),
    oneway void putObject(1: PutObjReq req)
}

service AMSvc extends BaseAsyncSvc {
}

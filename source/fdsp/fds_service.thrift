/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
namespace cpp FDS_ProtocolInterface

/*
 * Service/domain identifiers.
 */
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
    2: required string        domain_name,
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

/*
 * Generic response message format.
 */
struct RespHdr {
    1: required i32           status,
    2: required string        text,
}

struct GetObjReq {
    1: required AsyncHdr      header,
    2: required string        id,
}
struct PutObjReq {
    1: required AsyncHdr      header,
    2: required string        id,
    3: required string        data,
}

struct GetObjRsp {
	1: required i32           len,
    2: required string        etag,
	3: required string        data,
}

struct PutObjRsp {
	1: required i32           status,
    2: required string        etag,
    3: optional string        text,
}

/*
 * --------------------------------------------------------------------------------
 * Node endpoint/service registration handshake
 * --------------------------------------------------------------------------------
 */

/*
 * Uuid to physical location binding registration.
 */
struct UuidBindMsg {
    1: required AsyncHdr      header,
    2: required SvcID         svc_id,
    3: required string        svc_addr,
    4: required i32           scv_port,
}

/*
 * Node storage capability message format.
 */
struct StorCapMsg {
    1: i32                    disk_iops_max,
    2: i32                    disk_iops_min,
    3: double                 disk_capacity,
    4: i32                    disk_latency_max,
    5: i32                    disk_latency_min,
    6: i32                    ssd_iops_max,
    7: i32                    ssd_iops_min,
    8: double                 ssd_capacity,
    9: i32                    ssd_latency_max,
    10: i32                   ssd_latency_min,
    11: i32                   ssd_count,
    12: i32                   disk_type,
    13: i32                   disk_count,
}

/*
 * Node registration message format.
 */
struct NodeInfoMsg {
    1: required UuidBindMsg   node_loc,
    2: required DomainID      node_domain,
    3: required StorCapMsg    node_stor,
}

/*
 * --------------------------------------------------------------------------------
 * Common services
 * --------------------------------------------------------------------------------
 */
service BaseAsyncSvc {
	oneway void asyncReqt(1: AsyncHdr header),
	oneway void asyncResp(1: AsyncHdr header, 2: string payload),
    RespHdr uuidBind(1: UuidBindMsg msg)
}

service PlatNetSvc extends BaseAsyncSvc {
    list<UuidBindMsg> allUuidBinding(1: UuidBindMsg mine),
    list<NodeInfoMsg> notifyNodeInfo(1: NodeInfoMsg info),
    RespHdr notifyNodeUp(1: NodeInfoMsg info)
}

service SMSvc extends BaseAsyncSvc {
	oneway void getObject(1: GetObjReq req),
	oneway void putObject(1: PutObjReq req)
}

service AMSvc extends BaseAsyncSvc {
}

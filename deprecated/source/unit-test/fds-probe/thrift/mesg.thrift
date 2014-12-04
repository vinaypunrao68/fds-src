include "fds_service.thrift"

namespace cpp FDS_ProtocolInterface

struct ProbePutMsg {
    1: required fds_service.AsyncHdr hdr,
    2: string msg_data,
}

struct ProbeGetMsgReqt {
    1: required fds_service.AsyncHdr hdr,
    2: i32    msg_len,
}

struct ProbeGetMsgResp {
    1: required fds_service.AsyncHdr hdr,
    2: i32    msg_len,
    3: string msg_data,
}

struct ProbeFoo {
    1: required fds_service.AsyncHdr hdr,
    2: required ProbeGetMsgReqt      reqt,
    3: optional i32      p1,
    4: optional i32      p2,
    5: optional string   p3,
}

struct ProbeBar {
    1: required fds_service.AsyncHdr hdr,
    3: optional i32      p1,
    4: optional i32      p2,
    5: optional i32      p3,
    6: optional i32      p4,
    7: optional i32      p5,
}

service ProbeServiceSM extends fds_service.BaseAsyncSvc {
    /* Required for async message passing semantic */
    oneway void msg_async_resp(1: fds_service.AsyncHdr orig,
                               2: fds_service.AsyncHdr resp),

    /* Regular RPC semantic */
    oneway void foo(1: ProbeFoo f),
    oneway void bar(1: ProbeBar b),
    oneway void probe_put(1: ProbePutMsg reqt),
    ProbeGetMsgResp probe_get(1: ProbeGetMsgReqt reqt),
}

struct ProbeAmCreatVol {
    1: required fds_service.AsyncHdr hdr,
    2: required fds_service.SvcUuid  vol_uuid,
    3: required string               vol_name,
    4: required string               vol_text,
}

struct ProbeAmCreatVolResp {
    1: required fds_service.AsyncHdr hdr,
    2: required fds_service.SvcUuid  vol_uuid,
    3: required i64                  vol_mb_size,
}

service ProbeServiceAM extends fds_service.BaseAsyncSvc {
    /* Required for async message passing semantic */
    oneway void msg_async_resp(1: fds_service.AsyncHdr orig,
                               2: fds_service.AsyncHdr resp),

    /* Regular RPC semantic */
    ProbeAmCreatVolResp am_creat_vol(1: ProbeAmCreatVol cmd),
    oneway void am_probe_put_resp(1: ProbeGetMsgResp resp),

    /*Probe foo */
    oneway void foo(1: ProbeFoo f),
}

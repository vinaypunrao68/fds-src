include "fds_service.thrift"

namespace cpp FDS_ProtocolInterface

struct ProbePutMsg {
    1: string msg_data,
}

struct ProbeGetMsgReqt {
    1: i32    msg_len,
}

struct ProbeGetMsgResp {
    1: i32    msg_len,
    2: string msg_data,
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

service ProbeService {
    /* Required for async message passing semantic */
    oneway void foo(1: ProbeFoo f),
    oneway void bar(1: ProbeBar b),
    oneway void msg_async_resp(1: fds_service.AsyncHdr    orig,
                               2: fds_service.AsyncRspHdr resp),

    /* Regular RPC semantic */
    oneway void probe_put(1: ProbePutMsg reqt),
    ProbeGetMsgResp probe_get(1: ProbeGetMsgReqt reqt),
}

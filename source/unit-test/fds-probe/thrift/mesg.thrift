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

service ProbeService {
    oneway void probe_put(1: ProbePutMsg reqt),
    ProbeGetMsgResp probe_get(1: ProbeGetMsgReqt reqt),
}

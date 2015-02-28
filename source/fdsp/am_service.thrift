/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "FDSP.thrift"
include "fds_service.thrift"
include "pm_service.thrift"
include "dm_service.thrift"
include "sm_service.thrift"

namespace cpp FDS_ProtocolInterface

/**
 * AM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service AMSvc extends fds_service.PlatNetSvc {
}

/* ---------------------  CtrlNotifyThrottleTypeId  ---------------------------- */
struct CtrlNotifyThrottle {
     1: FDSP.FDSP_ThrottleMsgType      throttle;
}

struct CtrlNotifyQoSControl {
     1: FDSP.FDSP_QoSControlMsgType    qosctrl;
}

service TestAMSvc {
    i32 associate(1: string myip, 2: i32 port);
    oneway void putObjectRsp(1: fds_service.AsyncHdr asyncHdr, 2: sm_service.PutObjectRspMsg payload);
    oneway void updateCatalogRsp(1: fds_service.AsyncHdr asyncHdr, 2: dm_service.UpdateCatalogOnceRspMsg payload);
}

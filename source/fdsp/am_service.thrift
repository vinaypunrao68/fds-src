/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "FDSP.thrift"
include "fds_service.thrift"
include "pm_service.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.am

/**
 * AM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service AMSvc extends pm_service.PlatNetSvc {
}

/* ---------------------  CtrlNotifyThrottleTypeId  ---------------------------- */
struct CtrlNotifyThrottle {
     1: FDSP.FDSP_ThrottleMsgType      throttle;
}

struct CtrlNotifyQoSControl {
     1: FDSP.FDSP_QoSControlMsgType    qosctrl;
}

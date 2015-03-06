/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "am_types.thrift"

include "common.thrift"
include "fds_service.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.am

/* ------------------------------------------------------------
   Operations on QoS
   ------------------------------------------------------------*/

/**
 * Set domain throttling level.
 */
struct CtrlNotifyThrottle {
  /** Domain throttle specification */
  1: common.FDSP_ThrottleMsgType        throttle;
}

/**
 * QoS Rate Control
 */
struct CtrlNotifyQoSControl {
  /** QoS rate specification. */
  1: am_types.FDSP_QoSControlMsgType    qosctrl;
}

/* ------------------------------------------------------------
   Other specified services
   ------------------------------------------------------------*/

/**
 * AM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service AMSvc extends fds_service.PlatNetSvc {
}

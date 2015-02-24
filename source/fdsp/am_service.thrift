/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

include "fds_service.thrift"

namespace cpp FDS_ProtocolInterface

/**
 * AM Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service AMSvc extends fds_service.PlatNetSvc {
}


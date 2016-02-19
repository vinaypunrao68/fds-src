/*
 * Copyright 2014-2016 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "common.thrift"
include "svc_api.thrift"
include "svc_types.thrift"

namespace cpp FDS_ProtocolInterface

/**
 * VC Service.  Only put sync rpc calls in here.  Async RPC calls use
 * message passing provided by BaseAsyncSvc
 */
service VCSvc extends svc_api.PlatNetSvc {
}


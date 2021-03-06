/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

include "svc_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.pm

/**
 * @param services a list of svc_types.SvcInfo representing the services to start
 *
 * @return nothing
 */
struct NotifyStartServiceMsg {
    1: list<svc_types.SvcInfo> services,
    2: bool isActionNodeStart;
    3: optional bool force = false;
}

/**
 * @param services a list of svc_types.SvcInfo representing the services to stop
 *
 * @return nothing
 */
struct NotifyStopServiceMsg {
    1: list<svc_types.SvcInfo> services,
    2: bool isActionNodeShutdown;
}

/**
 * @param services a list of svc_types.SvcInfo representing the services to add
 *
 * @return nothing
 */
struct NotifyAddServiceMsg {
    1: list<svc_types.SvcInfo> services;
}

/**
 * @param services a list of svc_types.SvcInfo representing the services to remove
 *
 * @return nothing
 */
struct NotifyRemoveServiceMsg {
    1: list<svc_types.SvcInfo> services,
    2: bool isActionNodeRemove;
}

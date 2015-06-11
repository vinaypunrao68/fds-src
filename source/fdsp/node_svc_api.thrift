/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

include "svc_types.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.pm

/*
 *****************************************************************************
 * NOTE:
 *
 * NotifyStartNodeMsg only starts well known services. The PM and OM will not be
 * started.
 *
 * NotifyStopNodeMsg only stops well known services. The PM and OM will not be
 * stopped.
 *
 * Neither of the NofityNode(Start|Stop)Msg will have any effect on the host
 * operating system.
 *****************************************************************************
 */

/**
 * @param nodes a list of svc_types.SvcInfo representing the nodes to start
 *
 * @return nothing
 */
struct NotifyStartNodeMsg {
    1: list<svc_types.SvcInfo> nodes;
}

/**
 * @param nodes a list of svc_types.SvcInfo representing the nodes to stop
 *
 * @return nothing
 */
struct NotifyStopNodeMsg {
    1: list<svc_types.SvcInfo> nodes;
}

/**
 * @param nodes a list of svc_types.SvcInfo representing the nodes to add
 *
 * @return nothing
 */
struct NotifyAddNodeMsg {
    1: list<svc_types.SvcInfo> nodes;
}

/**
 * @param nodes a list of svc_types.SvcInfo representing the nodes to remove
 *
 * @return nothing
 */
struct NotifyRemoveNodeMsg {
    1: list<svc_types.SvcInfo> nodes;
}

/**
 * @param services a list of svc_types.SvcInfo representing the services to start
 *
 * @return nothing
 */
struct NotifyStartServiceMsg {
    1: list<svc_types.SvcInfo> services;
}

/**
 * @param services a list of svc_types.SvcInfo representing the services to stop
 *
 * @return nothing
 */
struct NotifyStopServiceMsg {
    1: list<svc_types.SvcInfo> services;
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
    1: list<svc_types.SvcInfo> services;
}

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
 * NotifyStartNode only starts well known services. The PM and OM will not be
 * started.
 *
 * NotifyStopNode only stops well known services. The PM and OM will not be
 * stopped.
 *
 * Neither of the NofityNode(Start|Stop) will have any effect on the host
 * operating system.
 *****************************************************************************
 */

/**
 * @param nodes a list of svc_types.SvcInfo representing the nodes to start
 *
 * @return nothing
 */
struct NotifyStartNode {
    1: list<svc_types.SvcInfo> nodes;
}

/**
 * @param nodes a list of svc_types.SvcInfo representing the nodes to stop
 *
 * @return nothing
 */
struct NotifyStopNode {
    1: list<svc_types.SvcInfo> nodes;
}

/**
 * @param nodes a list of svc_types.SvcInfo representing the nodes to add
 *
 * @return nothing
 */
struct NotifyAddNode {
    1: list<svc_types.SvcInfo> nodes;
}

/**
 * @param nodes a list of svc_types.SvcInfo representing the nodes to remove
 *
 * @return nothing
 */
struct NotifyRemoveNode {
    1: list<svc_types.SvcInfo> nodes;
}

/**
 * @param services a list of svc_types.SvcInfo representing the services to start
 *
 * @return nothing
 */
struct NotifyStartService {
    1: list<svc_types.SvcInfo> services;
}

/**
 * @param services a list of svc_types.SvcInfo representing the services to stop
 *
 * @return nothing
 */
struct NotifyStopService {
    1: list<svc_types.SvcInfo> services;
}

/**
 * @param services a list of svc_types.SvcInfo representing the services to add
 *
 * @return nothing
 */
struct NotifyAddService {
    1: list<svc_types.SvcInfo> services;
}

/**
 * @param services a list of svc_types.SvcInfo representing the services to remove
 *
 * @return nothing
 */
struct NotifyRemoveService {
    1: list<svc_types.SvcInfo> services;
}
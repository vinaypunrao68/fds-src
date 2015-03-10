/*
 * Copyright 2014-2015 by Formation Data Systems, Inc.
 */

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.pm

include "FDSP.thrift"
include "pm_types.thrift"
include "common.thrift"
include "svc_api.thrift"

service PlatNetSvc extends svc_api.BaseAsyncSvc {
    oneway void allUuidBinding(1: svc_api.UuidBindMsg mine);
    oneway void notifyNodeActive(1: FDSP.FDSP_ActivateNodeType info);

    list<pm_types.NodeInfoMsg> notifyNodeInfo(1: pm_types.NodeInfoMsg info, 2: bool bcast);
    pm_types.DomainNodes getDomainNodes(1: pm_types.DomainNodes dom);
    pm_types.NodeEvent getSvcEvent(1: pm_types.NodeEvent input);

    pm_types.ServiceStatus getStatus(1: i32 nullarg);
    map<string, i64> getCounters(1: string id);
    void resetCounters(1: string id);
    void setConfigVal(1:string id, 2:i64 value);
    void setFlag(1:string id, 2:i64 value);
    i64 getFlag(1:string id);
    map<string, i64> getFlags(1: i32 nullarg);
    /* For setting fault injection.
     * @param cmdline format based on libfiu
     */
    bool setFault(1: string cmdline);
}
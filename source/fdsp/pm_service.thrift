/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

include "FDSP.thrift"
include "fds_service.thrift"
include "common.thrift"

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol.pm

/*
 * --------------------------------------------------------------------------------
 * Node endpoint/service registration handshake
 * --------------------------------------------------------------------------------
 */
enum ServiceStatus {
    SVC_STATUS_INVALID      = 0x0000,
    SVC_STATUS_ACTIVE       = 0x0001,
    SVC_STATUS_INACTIVE     = 0x0002,
    SVC_STATUS_IN_ERR       = 0x0004
}

/**
 * Service map info
 */
struct SvcInfo {
    1: required common.SvcID             svc_id,
    2: required i32                      svc_port,
    3: required FDSP.FDSP_MgrIdType      svc_type,
    4: required ServiceStatus            svc_status,
    5: required string                   svc_auto_name,
}

/**
 * This becomes a control path message
 */
struct NodeSvcInfo {
    1: required common.SvcUuid           node_base_uuid,
    2: i32                               node_base_port,
    3: string                            node_addr,
    4: string                            node_auto_name,
    5: FDSP.FDSP_NodeState               node_state;
    6: i32                               node_svc_mask,
    7: list<SvcInfo>                     node_svc_list,
}

struct DomainNodes {
    1: required common.DomainID          dom_id,
    2: list<NodeSvcInfo>                 dom_nodes,
}

struct StorCapMsg {
    1: i32                    disk_iops_max,
    2: i32                    disk_iops_min,
    3: double                 disk_capacity,
    4: i32                    disk_latency_max,
    5: i32                    disk_latency_min,
    6: i32                    ssd_iops_max,
    7: i32                    ssd_iops_min,
    8: double                 ssd_capacity,
    9: i32                    ssd_latency_max,
    10: i32                   ssd_latency_min,
    11: i32                   ssd_count,
    12: i32                   disk_type,
    13: i32                   disk_count,
}

/**
 * Work item done by a node.
 */
struct NodeWorkItem {
    1: required i32                      nd_work_code,
    2: required common.DomainID          nd_dom_id,
    3: required common.SvcUuid           nd_from_svc,
    4: required common.SvcUuid           nd_to_svc,
}

struct NodeDeploy {
    1: required common.DomainID          nd_dom_id,
    2: required common.SvcUuid           nd_uuid,
    3: list<NodeWorkItem>                nd_work_item,
}

struct NodeDown {
    1: required common.DomainID          nd_dom_id,
    2: required common.SvcUuid           nd_uuid,
}

/**
 * Events emit from a node.
 */
struct NodeEvent {
    1: required common.DomainID          nd_dom_id,
    2: required common.SvcUuid           nd_uuid,
    3: required string                   nd_evt,
    4: string                            nd_evt_text,
}

struct NodeFunctional {
    1: required common.DomainID          	nd_dom_id,
    2: required common.SvcUuid           	nd_uuid,
    3: required fds_service.FDSPMsgTypeId	nd_op_code,
    4: list<NodeWorkItem>                	nd_work_item,
}

/*
 * Node registration message format.
 */
struct NodeInfoMsg {
    1: required fds_service.UuidBindMsg node_loc,
    2: required common.DomainID		node_domain,
    3: StorCapMsg 			node_stor,
    4: required i32           		nd_base_port,
    5: required i32           		nd_svc_mask,
    6: required bool          		nd_bcast,
}

struct NodeIntegrate {
    1: required common.DomainID          nd_dom_id,
    2: required common.SvcUuid           nd_uuid,
    3: bool                              nd_start_am,
    4: bool                              nd_start_dm,
    5: bool                              nd_start_sm,
    6: bool                              nd_start_om,
}

struct NodeVersion {
    1: required i32                      nd_fds_maj,
    2: required i32                      nd_fds_min,
    3: i32                               nd_fds_release,
    4: i32                               nd_fds_patch,
    5: string                            nd_java_ver,
}

/**
 * Qualify a node before admitting it to the domain.
 */
struct NodeQualify {
    1: required NodeInfoMsg              nd_info,
    2: required string                   nd_acces_token,
    3: NodeVersion                       nd_sw_ver,
    4: string                            nd_md5sum_pm,
    5: string                            nd_md5sum_sm,
    6: string                            nd_md5sum_dm,
    7: string                            nd_md5sum_am,
    8: string                            nd_md5sum_om,
}

/**
 * Notify node to upgrade/rollback SW version.
 */
struct NodeUpgrade {
    1: required common.DomainID          	nd_dom_id,
    2: required common.SvcUuid           	nd_uuid,
    3: NodeVersion                       	nd_sw_ver,
    4: required fds_service.FDSPMsgTypeId	nd_op_code,
    5: required string                   	nd_md5_chksum,
    6: string                            	nd_pkg_path,
}

service PlatNetSvc extends fds_service.BaseAsyncSvc {
    oneway void allUuidBinding(1: fds_service.UuidBindMsg mine);
    oneway void notifyNodeActive(1: FDSP.FDSP_ActivateNodeType info);

    list<NodeInfoMsg> notifyNodeInfo(1: NodeInfoMsg info, 2: bool bcast);
    DomainNodes getDomainNodes(1: DomainNodes dom);
    NodeEvent   getSvcEvent(1: NodeEvent input);

    ServiceStatus getStatus(1: i32 nullarg);
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

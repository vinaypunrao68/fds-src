/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <vector>
#include <string>
#include <stdlib.h>
#include <dlt.h>
#include <fds-shmobj.h>
#include <fdsp/PlatNetSvc.h>
#include <apis/ConfigurationService.h>
#include <net/net-service-tmpl.hpp>
#include <net/SvcRequestPool.h>
#include <platform/platform-lib.h>
#include <platform/node-inv-shmem.h>
#include <platform/service-ep-lib.h>
#include <platform/plat-serialize.h>
#include <NetSession.h>

namespace fds {

const NodeUuid               gl_OmPmUuid(0xcac0);
const NodeUuid               gl_OmUuid(0xcac0 | fpi::FDSP_ORCH_MGR);

// --------------------------------------------------------------------------------------
// Node Inventory
// --------------------------------------------------------------------------------------
const ShmObjRO *
NodeInventory::node_shm_ctrl() const
{
    if (node_svc_type == fpi::FDSP_STOR_HVISOR) {
        return NodeShmCtrl::shm_am_inventory();
    }
    return NodeShmCtrl::shm_node_inventory();
}

// node_get_shm_rec
// ----------------
// Return the copy of node data in shared memory.
//
void
NodeInventory::node_get_shm_rec(node_data_t *ndata) const
{
    int             idx;
    NodeUuid        nd_uuid;
    fds_uint64_t    uuid;
    const ShmObjRO *shm;

    rs_uuid.uuid_get_base_val(&nd_uuid);
    if (node_ro_idx == -1) {
        /* TODO(Vy): support legacy OM path. */
        Platform *plat = Platform::platf_singleton();
        NodeAgent::pointer na = plat->plf_find_node_agent(nd_uuid);

        fds_verify((na != NULL) && (na != this) && (na->node_ro_idx != -1));
        na->node_get_shm_rec(ndata);
        return;
    }
    shm  = NodeShmCtrl::shm_node_inventory();
    uuid = nd_uuid.uuid_get_val();
    idx  = shm->shm_lookup_rec(node_ro_idx, static_cast<const void *>(&uuid),
                               static_cast<void *>(ndata), sizeof(*ndata));

    fds_verify(idx == node_ro_idx);
}

// node_info_frm_shm
// -----------------
//
void
NodeInventory::node_info_frm_shm(node_data_t *out) const
{
    int             idx;
    NodeUuid        uuid;
    fds_uint64_t    uid;
    const ShmObjRO *shm;

    rs_uuid.uuid_get_base_val(&uuid);
    if (node_ro_idx == -1) {
        /* TODO(Vy): support legacy OM path. */
        Platform *plat = Platform::platf_singleton();
        NodeAgent::pointer na = plat->plf_find_node_agent(uuid);

        fds_verify((na != NULL) && (na != this) && (na->node_ro_idx != -1));
        na->node_info_frm_shm(out);
        return;
    }
    uid = uuid.uuid_get_val();
    shm = node_shm_ctrl();
    idx = shm->shm_lookup_rec(node_ro_idx, static_cast<const void *>(&uid),
                              static_cast<void *>(out), sizeof(*out));
    fds_assert(idx == node_ro_idx);
}

#if 0
// get_node_name
// -------------
//
std::string
NodeInventory::get_node_name() const
{
    node_data_t ndata;

    node_get_shm_rec(&ndata);
    return std::string(ndata.nd_auto_name);
}
#endif

// get_ip_str
// ----------
//
std::string
NodeInventory::get_ip_str() const
{
    node_data_t ndata;

    node_get_shm_rec(&ndata);
    return std::string(ndata.nd_ip_addr);
}

fds_uint32_t
NodeInventory::node_base_port() const
{
    node_data_t ndata;

    node_get_shm_rec(&ndata);
    return Platform::plf_svc_port_from_node(ndata.nd_base_port, node_svc_type);
}

// get_node_root
// -------------
//
std::string
NodeInventory::get_node_root() const
{
    return node_root;
}

// node_capability
// ---------------
//
const node_stor_cap_t *
NodeInventory::node_capability() const
{
    const ShmObjRO    *shm;
    const node_data_t *ndata;

    shm   = node_shm_ctrl();
    if (node_ro_idx == -1) {
        /* TODO(Vy): support OM legacy path where we have 2 node agents. */
        NodeAgent::pointer  na;
        NodeUuid   uuid;
        Platform  *plat = Platform::platf_singleton();

        rs_uuid.uuid_get_base_val(&uuid);
        na = plat->plf_find_node_agent(uuid);
        fds_verify((na != NULL) && (na != this) && (na->node_ro_idx != -1));
        return na->node_capability();
    }
    ndata = shm->shm_get_rec<node_data_t>(node_ro_idx);
    fds_verify(ndata != NULL);
    return &ndata->nd_capability;
}

// node_fill_shm_inv
// -----------------
//
void
NodeInventory::node_fill_shm_inv(const ShmObjRO *shm, int ro, int rw, FdspNodeType id)
{
    const node_data_t *info;

    fds_assert(ro != -1);
    node_svc_type = id;

    info = shm->shm_get_rec<node_data_t>(ro);
    if (node_ro_idx == -1) {
        strncpy(rs_name, info->nd_auto_name, RS_NAME_MAX - 1);
    } else {
        NodeUuid svc, node(info->nd_node_uuid);

        Platform::plf_svc_uuid_from_node(node, &svc, node_svc_type);
        fds_assert(rs_uuid == svc);
    }
    node_ro_idx = ro;
    node_rw_idx = rw;
}

// node_info_msg_to_shm
// --------------------
// Convert data from the node info message to record to save to the shared memory seg.
//
/* static */ void
NodeInventory::node_info_msg_to_shm(const fpi::NodeInfoMsg *msg, node_data_t *rec)
{
    const fpi::UuidBindMsg *msg_bind;

    msg_bind             = &msg->node_loc;
    rec->nd_node_uuid    = msg_bind->svc_node.svc_uuid.svc_uuid;
    rec->nd_service_uuid = msg_bind->svc_id.svc_uuid.svc_uuid;
    rec->nd_base_port    = msg->nd_base_port;
    rec->nd_svc_type     = fpi::FDSP_PLATFORM;
    rec->nd_node_state   = fpi::FDS_Node_Discovered;
    rec->nd_dlt_version  = DLT_VER_INVALID;
    rec->nd_disk_type    = msg->node_stor.disk_type;

    node_stor_cap_to_shm(&msg->node_stor, &rec->nd_capability);
    strncpy(rec->nd_ip_addr, msg_bind->svc_addr.c_str(), FDS_MAX_IP_STR);
    strncpy(rec->nd_auto_name, msg_bind->svc_auto_name.c_str(), FDS_MAX_NODE_NAME);
    strncpy(rec->nd_assign_name, msg_bind->svc_id.svc_name.c_str(), FDS_MAX_NODE_NAME);
}

// node_info_msg_frm_shm
// ---------------------
// Convert data from shm at the given index to the node info message.
//
/* static */ void
NodeInventory::node_info_msg_frm_shm(bool am, int ro_idx, fpi::NodeInfoMsg *msg)
{
    fpi::UuidBindMsg      *msg_bind;
    const ShmObjRO        *shm;
    const node_data_t     *rec;

    if (am == true) {
        shm = NodeShmCtrl::shm_am_inventory();
    } else {
        shm = NodeShmCtrl::shm_node_inventory();
    }
    fds_verify(ro_idx != -1);
    rec = shm->shm_get_rec<node_data_t>(ro_idx);

    msg_bind = &msg->node_loc;
    msg_bind->svc_id.svc_uuid.svc_uuid   = rec->nd_service_uuid;
    msg_bind->svc_node.svc_uuid.svc_uuid = rec->nd_node_uuid;
    msg_bind->svc_type                   = rec->nd_svc_type;
    msg->nd_base_port                    = rec->nd_base_port;

    msg_bind->svc_addr.assign(rec->nd_ip_addr);
    msg_bind->svc_auto_name.assign(rec->nd_auto_name);
    msg_bind->svc_id.svc_name.assign(rec->nd_assign_name);
    node_stor_cap_frm_shm(&msg->node_stor, &rec->nd_capability);
}

// init_plat_info_msg
// ------------------
// Format the NodeInfoMsg that will be processed by functions above.  The data is taken
// from the platform lib to represent the HW node running the SW.
//
void
NodeInventory::init_plat_info_msg(fpi::NodeInfoMsg *msg) const
{
    fpi::UuidBindMsg   *msg_bind;
    Platform::ptr       plat = Platform::platf_const_singleton();
    EpSvcImpl::pointer  psvc = NetPlatform::nplat_singleton()->nplat_my_ep();

    msg_bind = &msg->node_loc;
    psvc->ep_fmt_uuid_binding(msg_bind, &msg->node_domain);
    init_stor_cap_msg(&msg->node_stor);

    msg->nd_base_port           = plat->plf_get_my_node_port();
    msg_bind->svc_type          = plat->plf_get_node_type();
    msg_bind->svc_auto_name     = *plat->plf_get_my_ip();
    msg_bind->svc_id.svc_name   = "";
    msg_bind->svc_node.svc_name = *plat->plf_get_my_name();
    msg_bind->svc_node.svc_uuid.svc_uuid = plat->plf_get_my_node_uuid()->uuid_get_val();
}

// init_om_info_msg
// ----------------
// Format the NodeInfoMsg with data about OM node.
void
NodeInventory::init_om_info_msg(fpi::NodeInfoMsg *msg)
{
    fpi::UuidBindMsg   *msg_bind;
    Platform::ptr       plat = Platform::platf_const_singleton();

    msg_bind                    = &msg->node_loc;
    msg->nd_base_port           = plat->plf_get_om_ctrl_port();
    msg_bind->svc_port          = plat->plf_get_om_ctrl_port();
    msg_bind->svc_addr          = *plat->plf_get_om_ip();
    msg_bind->svc_type          = fpi::FDSP_ORCH_MGR;
    msg_bind->svc_auto_name     = "OM";
    msg_bind->svc_id.svc_name   = "OM";
    msg_bind->svc_node.svc_name = *plat->plf_get_my_name();

    msg_bind->svc_id.svc_uuid.svc_uuid   = gl_OmUuid.uuid_get_val();
    msg_bind->svc_node.svc_uuid.svc_uuid = gl_OmUuid.uuid_get_val();
    memset(&msg->node_stor, 0, sizeof(msg->node_stor));
}

// init_om_pm_info_msg
// -------------------
// Format the NodeInfoMsg with data about OM Platform Services.
void
NodeInventory::init_om_pm_info_msg(fpi::NodeInfoMsg *msg)
{
    fpi::UuidBindMsg   *msg_bind;
    Platform::ptr       plat = Platform::platf_const_singleton();

    msg_bind                    = &msg->node_loc;
    msg->nd_base_port           = plat->plf_get_om_svc_port();
    msg_bind->svc_port          = plat->plf_get_om_ctrl_port();
    msg_bind->svc_addr          = *plat->plf_get_om_ip();
    msg_bind->svc_type          = fpi::FDSP_PLATFORM;
    msg_bind->svc_auto_name     = "OM-PM";
    msg_bind->svc_id.svc_name   = "OM-PM";
    msg_bind->svc_node.svc_name = *plat->plf_get_my_name();

    msg_bind->svc_id.svc_uuid.svc_uuid   = gl_OmPmUuid.uuid_get_val();
    msg_bind->svc_node.svc_uuid.svc_uuid = gl_OmPmUuid.uuid_get_val();
    init_stor_cap_msg(&msg->node_stor);
}

// init_node_info_msg
// ------------------
// Format the NodeInfoMsg with data from this node agent obj.
void
NodeInventory::init_node_info_msg(fpi::NodeInfoMsg *msg) const
{
    node_info_msg_frm_shm(node_svc_type == fpi::FDSP_STOR_HVISOR ? true : false,
                          node_ro_idx, msg);
}

// node_stor_cap_to_shm
// --------------------
// Format storage capability info in shared memory from a network message.
//
/* static */ void
NodeInventory::node_stor_cap_to_shm(const fpi::StorCapMsg *msg, node_stor_cap_t *stor)
{
    stor->disk_capacity    = msg->disk_capacity;
    stor->disk_iops_max    = msg->disk_iops_max;
    stor->disk_iops_min    = msg->disk_iops_min;
    stor->disk_latency_max = msg->disk_latency_max;
    stor->disk_latency_min = msg->disk_latency_min;
    stor->ssd_iops_max     = msg->ssd_iops_max;
    stor->ssd_iops_min     = msg->ssd_iops_min;
    stor->ssd_capacity     = msg->ssd_capacity;
    stor->ssd_latency_max  = msg->ssd_latency_max;
    stor->ssd_latency_min  = msg->ssd_latency_min;
}

// node_stor_cap_frm_shm
// ---------------------
// Format storage capability info to a network message from data in shared memory.
//
/* static */ void
NodeInventory::node_stor_cap_frm_shm(fpi::StorCapMsg *msg, const node_stor_cap_t *stor)
{
    msg->disk_capacity    = stor->disk_capacity;
    msg->disk_iops_max    = stor->disk_iops_max;
    msg->disk_iops_min    = stor->disk_iops_min;
    msg->disk_latency_max = stor->disk_latency_max;
    msg->disk_latency_min = stor->disk_latency_min;
    msg->ssd_iops_max     = stor->ssd_iops_max;
    msg->ssd_iops_min     = stor->ssd_iops_min;
    msg->ssd_capacity     = stor->ssd_capacity;
    msg->ssd_latency_max  = stor->ssd_latency_max;
    msg->ssd_latency_min  = stor->ssd_latency_min;
}

// init_stor_cap_msg
// -----------------
//
void
NodeInventory::init_stor_cap_msg(fpi::StorCapMsg *msg) const
{
    const ShmObjRO    *shm;
    const node_data_t *rec;

    if (node_ro_idx == -1) {
        /* Don't have real numbers, made up some values. */
        msg->disk_iops_max     = 10000;
        msg->disk_iops_min     = 4000;
        msg->disk_capacity     = 0x7ffff;
        msg->disk_latency_max  = 1000000 / msg->disk_iops_min;
        msg->disk_latency_min  = 1000000 / msg->disk_iops_max;
        msg->ssd_iops_max      = 200000;
        msg->ssd_iops_min      = 20000;
        msg->ssd_capacity      = 0x1000;
        msg->ssd_latency_max   = 1000000 / msg->ssd_iops_min;
        msg->ssd_latency_min   = 1000000 / msg->ssd_iops_max;
        msg->ssd_count         = 2;
        msg->disk_count        = 24;
        msg->disk_type         = FDS_DISK_SATA;
    } else {
        shm = node_shm_ctrl();
        rec = shm->shm_get_rec<node_data_t>(node_ro_idx);
        node_stor_cap_frm_shm(msg, &rec->nd_capability);
    }
}

// svc_info_frm_shm
// ----------------
//
void
NodeInventory::svc_info_frm_shm(fpi::SvcInfo *svc) const
{
    node_data_t ninfo;

    node_info_frm_shm(&ninfo);
    svc->svc_id.svc_uuid.svc_uuid = ninfo.nd_node_uuid;

    svc->svc_id.svc_name.assign(rs_name);
    svc->svc_auto_name.assign(ninfo.nd_auto_name);
    svc->svc_type   = node_svc_type;
    svc->svc_status = fpi::SVC_STATUS_ACTIVE;
}

// node_fill_inventory
// -------------------
// TODO(Vy): depricate this function.
//
void
NodeInventory::node_fill_inventory(const FdspNodeRegPtr msg)
{
    std::string ip;

    nd_gbyte_cap = msg->disk_info.ssd_capacity;
    nd_node_name = msg->node_name;
    node_root    = msg->node_root;

    ip = netSession::ipAddr2String(msg->ip_lo_addr);
    strncpy(rs_name, ip.c_str(), RS_NAME_MAX - 1);
}

// set_node_state
// --------------
//
void
NodeInventory::set_node_state(FdspNodeState state)
{
    nd_my_node_state = state;
}

void
NodeInventory::set_node_dlt_version(fds_uint64_t dlt_version)
{
    // TODO(Vy): do this in platform side.
    nd_my_dlt_version = dlt_version;
}

// node_update_inventory
// ---------------------
//
void
NodeInventory::node_update_inventory(const FdspNodeRegPtr msg)
{
}

// init_msg_hdr
// ------------
//
void
NodeInventory::init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const
{
    Platform::ptr plat = Platform::platf_const_singleton();

    msgHdr->minor_ver = 0;
    msgHdr->msg_id    = 1;
    msgHdr->major_ver = 0xa5;
    msgHdr->minor_ver = 0x5a;

    msgHdr->num_objects = 1;
    msgHdr->frag_len    = 0;
    msgHdr->frag_num    = 0;

    msgHdr->tennant_id      = 0;
    msgHdr->local_domain_id = 0;
    msgHdr->err_code        = ERR_OK;
    msgHdr->result          = FDS_ProtocolInterface::FDSP_ERR_OK;

    if (plat == NULL) {
        /* We'll get here if this is OM */
        msgHdr->src_id        = FDSP_ORCH_MGR;
        msgHdr->src_node_name = "";
    } else {
        msgHdr->src_id        = plat->plf_get_node_type();
        msgHdr->src_node_name = *plat->plf_get_my_name();
    }
}

// init_node_info_pkt
// ------------------
//
void
NodeInventory::init_node_info_pkt(fpi::FDSP_Node_Info_TypePtr pkt) const
{
    Platform     *plat = Platform::platf_singleton();
    fds_uint32_t  base = node_base_port();

    pkt->node_id        = 0;
    pkt->node_uuid      = Platform::plf_get_my_node_uuid()->uuid_get_val();
    pkt->service_uuid   = get_uuid().uuid_get_val();
    pkt->ip_hi_addr     = 0;
    pkt->ip_lo_addr     = str_to_ipv4_addr(get_ip_str());
    pkt->node_type      = node_get_svc_type();
    pkt->node_name      = get_node_name();
    pkt->node_state     = node_state();
    pkt->control_port   = plat->plf_get_my_ctrl_port(base);
    pkt->data_port      = plat->plf_get_my_data_port(base);
    pkt->migration_port = plat->plf_get_my_migration_port(base);
    pkt->metasync_port  = plat->plf_get_my_metasync_port(base);
    pkt->node_root      = get_node_root();
}

// init_node_reg_pkt
// -----------------
//
void
NodeInventory::init_node_reg_pkt(fpi::FDSP_RegisterNodeTypePtr pkt) const
{
    Platform::ptr plat = Platform::platf_const_singleton();
    pkt->node_type     = plat->plf_get_node_type();
    pkt->node_name     = *plat->plf_get_my_name();
    pkt->ip_hi_addr    = 0;
    pkt->ip_lo_addr    = str_to_ipv4_addr(*plat->plf_get_my_ip());
    pkt->control_port  = plat->plf_get_my_ctrl_port();
    pkt->node_root     = g_fdsprocess->proc_fdsroot()->dir_fdsroot();
}

uint32_t NodeServices::write(serialize::Serializer*  s) const {
    uint32_t b = 0;
    b += s->writeI64(sm.uuid_get_val());
    b += s->writeI64(dm.uuid_get_val());
    b += s->writeI64(am.uuid_get_val());
    return b;
}

uint32_t NodeServices::read(serialize::Deserializer* s) {
    uint32_t b = 0;
    fds_uint64_t uuid;
    b += s->readI64(uuid); sm = uuid;
    b += s->readI64(uuid); dm = uuid;
    b += s->readI64(uuid); am = uuid;
    return b;
}

std::ostream& operator<< (std::ostream &os, const NodeServices& node) {
    os << "["
       << " sm:" << node.sm
       << " dm:" << node.dm
       << " am:" << node.am
       << "]";
    return os;
}

// --------------------------------------------------------------------------------------
// Node Agents
// --------------------------------------------------------------------------------------

NodeAgent::~NodeAgent() {}
NodeAgent::NodeAgent(const NodeUuid &uuid)
    : NodeInventory(uuid), nd_eph(NULL),
      nd_ctrl_eph(NULL), nd_svc_rpc(NULL), nd_ctrl_rpc(NULL) {}

// node_stor_weight
// ----------------
//
fds_uint64_t
NodeAgent::node_stor_weight() const
{
    // lets normalize = nodes have same weight if their
    // capacity is within 10GB diff
    fds_uint64_t weight = nd_gbyte_cap / 10;
    if (weight < 1) {
        weight = 1;
    }
    return weight;
}

// node_set_weight
// ---------------
//
void
NodeAgent::node_set_weight(fds_uint64_t weight)
{
    nd_gbyte_cap = weight;
}

// agent_ep_plugin
// ---------------
//
EpEvtPlugin::pointer
NodeAgent::agent_ep_plugin()
{
    return NULL;
}

// agent_bind_ep
// -------------
//
void
NodeAgent::agent_bind_ep(EpSvcImpl::pointer ep, EpSvc::pointer svc)
{
    ep->ep_bind_service(svc);
}

// agent_publish_ep
// ----------------
//
void
NodeAgent::agent_publish_ep()
{
    int          port;
    NodeUuid     uuid;
    std::string  ip;
    fpi::SvcUuid svc;

    svc.svc_uuid = rs_uuid.uuid_get_val();
    port = NetMgr::ep_mgr_singleton()->ep_uuid_binding(svc, 0, 0, &ip);
    LOGDEBUG << "Agent lookup ep " << std::hex << svc.svc_uuid
        << ", obj " << this << ", svc type " << node_svc_type
        << ", idx " << node_ro_idx << ", rw idx " << node_rw_idx
        << ", ip " << ip << ":" << std::dec << port;
}

AgentContainer::AgentContainer(FdspNodeType id) : RsContainer()
{
    ac_id        = id;
    ac_cpSessTbl = boost::shared_ptr<netSessionTbl>(new netSessionTbl(id));
}

// agent_handshake
// ---------------
//
void
AgentContainer::agent_handshake(boost::shared_ptr<netSessionTbl> net,
                                NodeAgent::pointer               agent)
{
}

// node_agent_up
// -------------
//
void
NodeAgent::node_agent_up()
{
    if (nd_eph == NULL) {
        auto rpc = node_svc_rpc(&nd_eph);
    }
    if (nd_ctrl_eph == NULL) {
        auto rpc = node_ctrl_rpc(&nd_ctrl_eph);
    }
}

// node_agent_down
// ---------------
// We should not have concurrent call between node_agent_up and node_agent_down.
//
void
NodeAgent::node_agent_down()
{
    nd_eph      = NULL;
    nd_ctrl_eph = NULL;
}

// node_ctrl_rpc
// -------------
//
boost::shared_ptr<fpi::FDSP_ControlPathReqClient>
NodeAgent::node_ctrl_rpc(EpSvcHandle::pointer *handle)
{
    NetMgr              *net;
    fpi::SvcUuid         peer;
    EpSvcHandle::pointer eph;

    if (nd_ctrl_eph != NULL) {
        goto out;
    }
    peer.svc_uuid = rs_uuid.uuid_get_val();
    net = NetMgr::ep_mgr_singleton();
    eph = net->svc_get_handle<fpi::FDSP_ControlPathReqClient>(peer, 0, NET_SVC_CTRL);

    /* TODO(Vy): wire up the common event plugin to handle error. */
    if (eph != NULL) {
        nd_ctrl_eph = eph;
 out:
        *handle     = nd_ctrl_eph;
        nd_ctrl_rpc = nd_ctrl_eph->svc_rpc<fpi::FDSP_ControlPathReqClient>();
        fds_verify(nd_ctrl_rpc != NULL);
        return nd_ctrl_rpc;
    }
    *handle = NULL;
    return NULL;
}

// agent_rpc
// ---------
//
boost::shared_ptr<fpi::PlatNetSvcClient>
NodeAgent::node_svc_rpc(EpSvcHandle::pointer *handle)
{
    NetMgr              *net;
    fpi::SvcUuid         peer;
    EpSvcHandle::pointer eph;

    if (nd_eph != NULL) {
        goto out;
    }
    peer.svc_uuid = rs_uuid.uuid_get_val();
    net = NetMgr::ep_mgr_singleton();
    eph = net->svc_get_handle<fpi::PlatNetSvcClient>(peer, 0, 0);

    /* TODO(Vy): wire up the common event plugin to handle error. */
    if (eph != NULL) {
        nd_eph     = eph;
 out:
        *handle    = nd_eph;
        nd_svc_rpc = nd_eph->svc_rpc<fpi::PlatNetSvcClient>();
        fds_verify(nd_svc_rpc != NULL);
        return nd_svc_rpc;
    }
    *handle = NULL;
    return NULL;
}


// node_om_request
// ---------------
//
boost::shared_ptr<EPSvcRequest>
NodeAgent::node_om_request()
{
    fpi::SvcUuid om_uuid;

    gl_OmUuid.uuid_assign(&om_uuid);
    return gSvcRequestPool->newEPSvcRequest(om_uuid);
}

// node_msg_request
// ----------------
//
boost::shared_ptr<EPSvcRequest>
NodeAgent::node_msg_request()
{
    fpi::SvcUuid om_uuid;

    gl_OmUuid.uuid_assign(&om_uuid);
    return gSvcRequestPool->newEPSvcRequest(om_uuid);
}

// Debug operator
//
std::ostream &
operator << (std::ostream &os, const NodeAgent::pointer node)
{
    os << " agent: " << node.get() << std::hex
       << " [" << node->get_uuid().uuid_get_val() << "] " << std::dec;
    return os;
}

// agent_svc_fillin
// ----------------
//
void
NodeAgent::agent_svc_fillin(fpi::NodeSvcInfo    *out,
                            const node_data_t   *ninfo,
                            fpi::FDSP_MgrIdType  type) const
{
}

// --------------------------------------------------------------------------------------
// PM Agent
// --------------------------------------------------------------------------------------
PmAgent::~PmAgent() {}
PmAgent::PmAgent(const NodeUuid &uuid) : NodeAgent(uuid)
{
    pm_ep_svc = Platform::platf_singleton()->plat_new_pm_svc(this, 0, 0);
    NetMgr::ep_mgr_singleton()->ep_register(pm_ep_svc, false);
}

EpEvtPlugin::pointer
PmAgent::agent_ep_plugin()
{
    return pm_ep_svc->ep_evt_plugin();
}

// agent_svc_info
// --------------
//
void
PmAgent::agent_svc_info(fpi::NodeSvcInfo *out) const
{
    node_data_t   ninfo;

    node_info_frm_shm(&ninfo);
    out->node_addr.assign(ninfo.nd_ip_addr);
    out->node_auto_name.assign(ninfo.nd_auto_name);

    out->node_base_uuid.svc_uuid = rs_uuid.uuid_get_val();
    out->node_base_port = ninfo.nd_base_port;
    out->node_state     = fpi::FDS_Node_Up;
    out->node_svc_mask  = NODE_DO_PROXY_ALL_SVCS;

    agent_svc_fillin(out, &ninfo, fpi::FDSP_STOR_MGR);
    agent_svc_fillin(out, &ninfo, fpi::FDSP_DATA_MGR);
    agent_svc_fillin(out, &ninfo, fpi::FDSP_STOR_HVISOR);
    agent_svc_fillin(out, &ninfo, fpi::FDSP_ORCH_MGR);
    agent_svc_fillin(out, &ninfo, fpi::FDSP_PLATFORM);
}

// agent_svc_fillin
// ----------------
// Fill in service specific info based on the node info from shared memory.
//
void
PmAgent::agent_svc_fillin(fpi::NodeSvcInfo    *out,
                          const node_data_t   *ninfo,
                          fpi::FDSP_MgrIdType  type) const
{
    int           port;
    NodeUuid      base, svc;
    fpi::SvcInfo  data;
    fpi::SvcUuid  uuid;

    base.uuid_set_val(ninfo->nd_node_uuid);
    Platform::plf_svc_uuid_from_node(base, &svc, type);

    data.svc_id.svc_uuid.svc_uuid = svc.uuid_get_val();
    data.svc_id.svc_name.assign(ninfo->nd_assign_name);
    data.svc_auto_name.assign(ninfo->nd_auto_name);

    data.svc_status = SVC_STATUS_ACTIVE;
    data.svc_type   = type;
    data.svc_port   = Platform::plf_svc_port_from_node(ninfo->nd_base_port, type);

    out->node_svc_list.push_back(data);
}

// agent_ep_svc
// ------------
//
PmSvcEp::pointer
PmAgent::agent_ep_svc()
{
    return pm_ep_svc;
}

// agent_bind_ep
// -------------
//
void
PmAgent::agent_bind_ep()
{
    EpSvcImpl::pointer ep = NetPlatform::nplat_singleton()->nplat_my_ep();
    NodeAgent::agent_bind_ep(ep, pm_ep_svc);
}

// --------------------------------------------------------------------------------------
// SM Agent
// --------------------------------------------------------------------------------------
SmAgent::SmAgent(const NodeUuid &uuid) : NodeAgent(uuid), sm_sess(NULL), sm_reqt(NULL)
{
    node_svc_type = fpi::FDSP_STOR_MGR;
    sm_ep_svc     = Platform::platf_singleton()->plat_new_sm_svc(this, 0, 0);
    NetMgr::ep_mgr_singleton()->ep_register(sm_ep_svc, false);
}

SmAgent::~SmAgent()
{
    /* TODO(Vy): shutdown netsession and cleanup stuffs here */
}

// agent_ep_plugin
// ---------------
//
EpEvtPlugin::pointer
SmAgent::agent_ep_plugin()
{
    return sm_ep_svc->ep_evt_plugin();
}

// agent_ep_svc
// ------------
//
SmSvcEp::pointer
SmAgent::agent_ep_svc()
{
    return sm_ep_svc;
}

// agent_bind_ep
// -------------
//
void
SmAgent::agent_bind_ep()
{
    EpSvcImpl::pointer ep = NetPlatform::nplat_singleton()->nplat_my_ep();
    NodeAgent::agent_bind_ep(ep, sm_ep_svc);
}

// sm_handshake
// ------------
//
/* virtual */ void
SmAgent::sm_handshake(boost::shared_ptr<netSessionTbl> net)
{
    std::string   ip = get_ip_str();
    fds_uint32_t  base = node_base_port();
    Platform     *plat = Platform::platf_singleton();

    boost::shared_ptr<fpi::FDSP_DataPathRespIf> resp;
    sm_sess = net->startSession<netDataPathClientSession>(
            ip, plat->plf_get_my_data_port(base),
            fpi::FDSP_STOR_MGR, 1 /* just 1 channel */, resp);

    sm_reqt    = sm_sess->getClient();
    sm_sess_id = sm_sess->getSessionId();

    FDS_PLOG(g_fdslog) << "Handshake with SM: " << ip
        << ", port " << plat->plf_get_my_data_port(base) << ", sess id " << sm_sess_id;
}

NodeAgentDpClientPtr SmAgent::get_sm_client() {
    return sm_reqt;
}

std::string SmAgent::get_sm_sess_id() {
    return sm_sess_id;
}

SmContainer::SmContainer(FdspNodeType id) : AgentContainer(id)
{
    ac_id = fpi::FDSP_STOR_MGR;
}

// agent_handshake
// ---------------
//
void
SmContainer::agent_handshake(boost::shared_ptr<netSessionTbl> net,
                             NodeAgent::pointer               agent)
{
    SmAgent::pointer sm = agt_cast_ptr<SmAgent>(agent);
    sm->sm_handshake(net);
}

// --------------------------------------------------------------------------------------
// DmAgent
// --------------------------------------------------------------------------------------
DmAgent::~DmAgent() {}
DmAgent::DmAgent(const NodeUuid &uuid) : NodeAgent(uuid)
{
    node_svc_type = fpi::FDSP_DATA_MGR;
    dm_ep_svc     = Platform::platf_singleton()->plat_new_dm_svc(this, 0, 0);
    NetMgr::ep_mgr_singleton()->ep_register(dm_ep_svc, false);
}

// agent_ep_plugin
// ---------------
//
EpEvtPlugin::pointer
DmAgent::agent_ep_plugin()
{
    return dm_ep_svc->ep_evt_plugin();
}

// agent_ep_svc
// ------------
//
DmSvcEp::pointer
DmAgent::agent_ep_svc()
{
    return dm_ep_svc;
}

// agent_bind_ep
// -------------
//
void
DmAgent::agent_bind_ep()
{
    EpSvcImpl::pointer ep = NetPlatform::nplat_singleton()->nplat_my_ep();
    NodeAgent::agent_bind_ep(ep, dm_ep_svc);
}

// --------------------------------------------------------------------------------------
// AmAgent
// --------------------------------------------------------------------------------------
AmAgent::~AmAgent() {}
AmAgent::AmAgent(const NodeUuid &uuid) : NodeAgent(uuid)
{
    node_svc_type = fpi::FDSP_STOR_HVISOR;
    am_ep_svc     = Platform::platf_singleton()->plat_new_am_svc(this, 0, 0);
    NetMgr::ep_mgr_singleton()->ep_register(am_ep_svc, false);
}

// agent_ep_plugin
// ---------------
//
EpEvtPlugin::pointer
AmAgent::agent_ep_plugin()
{
    return am_ep_svc->ep_evt_plugin();
}

// agent_ep_svc
// ------------
//
AmSvcEp::pointer
AmAgent::agent_ep_svc()
{
    return am_ep_svc;
}

// agent_bind_ep
// -------------
//
void
AmAgent::agent_bind_ep()
{
    EpSvcImpl::pointer ep = NetPlatform::nplat_singleton()->nplat_my_ep();
    NodeAgent::agent_bind_ep(ep, am_ep_svc);
}

// --------------------------------------------------------------------------------------
// OM Agent
// --------------------------------------------------------------------------------------
OmAgent::~OmAgent() {}
OmAgent::OmAgent(const NodeUuid &uuid) : NodeAgent(uuid), om_sess(NULL), om_reqt(NULL)
{
    node_svc_type = fpi::FDSP_ORCH_MGR;
    om_ep_svc     = Platform::platf_singleton()->plat_new_om_svc(this, 0, 0);
    NetMgr::ep_mgr_singleton()->ep_register(om_ep_svc, false);
}

// agent_ep_plugin
// ---------------
//
EpEvtPlugin::pointer
OmAgent::agent_ep_plugin()
{
    return om_ep_svc->ep_evt_plugin();
}

// agent_ep_svc
// ------------
//
OmSvcEp::pointer
OmAgent::agent_ep_svc()
{
    return om_ep_svc;
}

// agent_bind_ep
// -------------
//
void
OmAgent::agent_bind_ep()
{
    EpSvcImpl::pointer ep = NetPlatform::nplat_singleton()->nplat_my_ep();
    NodeAgent::agent_bind_ep(ep, om_ep_svc);
}

// init_msg_hdr
// ------------
//
void
OmAgent::init_msg_hdr(fpi::FDSP_MsgHdrTypePtr hdr) const
{
    NodeInventory::init_msg_hdr(hdr);
    hdr->msg_code     = FDSP_MSG_PUT_OBJ_REQ;  // TODO(Vy): cleanup these codes.
    hdr->dst_id       = FDSP_ORCH_MGR;
    hdr->session_uuid = om_sess_id;
}

// init_node_reg_pkt
// -----------------
//
void
OmAgent::init_node_reg_pkt(fpi::FDSP_RegisterNodeTypePtr pkt) const
{
    Platform::ptr plat = Platform::platf_const_singleton();

    NodeInventory::init_node_reg_pkt(pkt);
    pkt->node_type         = plat->plf_get_node_type();
    pkt->node_uuid.uuid    = plat->plf_get_my_node_uuid()->uuid_get_val();
    pkt->service_uuid.uuid = pkt->node_uuid.uuid + 1 + pkt->node_type;
    pkt->node_name         = *plat->plf_get_my_name();
    pkt->ip_hi_addr        = 0;
    pkt->ip_lo_addr        = fds::str_to_ipv4_addr(*plat->plf_get_my_ip());
    pkt->control_port      = plat->plf_get_my_ctrl_port();
    pkt->data_port         = plat->plf_get_my_data_port();
    pkt->migration_port    = plat->plf_get_my_migration_port();
    pkt->metasync_port     = plat->plf_get_my_metasync_port();
}

// om_register_node
// ----------------
//
void
OmAgent::om_register_node(fpi::FDSP_RegisterNodeTypePtr reg)
{
    fpi::FDSP_MsgHdrTypePtr hdr(new fpi::FDSP_MsgHdrType);

    fds_verify(om_reqt != NULL);
    init_msg_hdr(hdr);
    om_reqt->RegisterNode(hdr, reg);
}

// om_handshake
// ------------
//
/* virtual */ void
OmAgent::om_handshake(boost::shared_ptr<netSessionTbl> net,
                      std::string                      om_ip,
                      fds_uint32_t                     om_port)
{
    boost::shared_ptr<fpi::FDSP_OMControlPathRespIf> resp;

    om_sess = net->startSession<netOMControlPathClientSession>(
            om_ip, om_port, fpi::FDSP_ORCH_MGR, 1 /* just 1 channel for now */, resp);

    om_reqt    = om_sess->getClient();
    om_sess_id = om_sess->getSessionId();
}

// get_om_config_svc
// ------------
//
// TODO(Vy): Make the necessary changes fit it to the svc endpoint model.
// Now it returns thrift client directly
boost::shared_ptr<apis::ConfigurationServiceClient> OmAgent::get_om_config_svc()
{
    using namespace apache::thrift;  // NOLINT
    using namespace apache::thrift::protocol; // NOLINT
    using namespace apache::thrift::transport; // NOLINT
    if (!om_cfg_svc) {
        boost::shared_ptr<TTransport> socket(new TSocket("localhost", 9090));
        boost::shared_ptr<TTransport> transport(new TFramedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        om_cfg_svc = boost::make_shared<apis::ConfigurationServiceClient>(protocol);
        transport->open();
    }
    return om_cfg_svc;
}


// --------------------------------------------------------------------------------------
// AgentContainer
// --------------------------------------------------------------------------------------
AgentContainer::~AgentContainer()
{
    ac_cpSessTbl->endAllSessions();
}

// agent_register
// --------------
//
Error
AgentContainer::agent_register(const NodeUuid       &uuid,
                               const FdspNodeRegPtr  msg,
                               NodeAgent::pointer   *out,
                               bool                  activate)
{
    fds_bool_t         add;
    std::string        name;
    NodeAgent::pointer agent;

    add   = false;
    *out  = NULL;
    name  = msg->node_name;
    agent = agt_cast_ptr<NodeAgent>(agent_info(uuid));
    if (agent == NULL) {
        add   = activate;
        agent = agt_cast_ptr<NodeAgent>(rs_alloc_new(uuid));
    }
    agent->node_fill_inventory(msg);
    *out = agent;
    if (add == true) {
        agent_activate(agent);
    }
    return Error(ERR_OK);
}

// agent_register
// --------------
//
void
AgentContainer::agent_register(const ShmObjRO     *shm,
                               NodeAgent::pointer *out,
                               int                 ro,
                               int                 rw)
{
    bool                add;
    NodeUuid            svc, node;
    const node_data_t  *info;
    NodeAgent::pointer  agent;

    add = true;
    if (*out == NULL) {
        info = shm->shm_get_rec<node_data_t>(ro);
        node.uuid_set_val(info->nd_service_uuid);
        Platform::plf_svc_uuid_from_node(node, &svc, ac_id);

        agent = agt_cast_ptr<NodeAgent>(agent_info(svc));
        if (agent == NULL) {
            agent = agt_cast_ptr<NodeAgent>(rs_alloc_new(svc));
        } else {
            add = false;
        }
        *out = agent;
    } else {
        agent = *out;
        fds_verify(agent->node_svc_type == ac_id);
    }
    agent->node_fill_shm_inv(shm, ro, rw, ac_id);

    if (add == true) {
        agent_activate(agent);
    }
    agent->agent_publish_ep();
}

// agent_unregister
// ----------------
//
Error
AgentContainer::agent_unregister(const NodeUuid &uuid, const std::string &name)
{
    NodeAgent::pointer  agent;

    agent = agent_info(uuid);
    if ((agent == NULL) ||
        (name.compare(agent->get_node_name()) != 0)) {
        return Error(ERR_NOT_FOUND);
    }
    agent_deactivate(agent);
    return Error(ERR_OK);
}

// --------------------------------------------------------------------------------------
// Node Container
// --------------------------------------------------------------------------------------

DomainContainer::~DomainContainer() {}
DomainContainer::DomainContainer(char const *const name)
    : dc_om_master(NULL), dc_om_nodes(NULL),
      dc_sm_nodes(NULL), dc_dm_nodes(NULL),
      dc_am_nodes(NULL), dc_pm_nodes(NULL) {}

DomainContainer::DomainContainer(char const *const       name,
                                 OmAgent::pointer        master,
                                 AgentContainer::pointer sm,
                                 AgentContainer::pointer dm,
                                 AgentContainer::pointer am,
                                 AgentContainer::pointer pm,
                                 AgentContainer::pointer om)
    : dc_om_master(master), dc_om_nodes(om),
      dc_sm_nodes(sm), dc_dm_nodes(dm), dc_am_nodes(am), dc_pm_nodes(pm) {}

// dc_container_frm_msg
// --------------------
// Get the right container based on message type.
//
AgentContainer::pointer
DomainContainer::dc_container_frm_msg(FdspNodeType node_type)
{
    AgentContainer::pointer nodes;

    switch (node_type) {
        case FDS_ProtocolInterface::FDSP_STOR_MGR:
            nodes = dc_sm_nodes;
            break;

        case FDS_ProtocolInterface::FDSP_DATA_MGR:
            nodes = dc_dm_nodes;
            break;

        case FDS_ProtocolInterface::FDSP_STOR_HVISOR:
            nodes = dc_am_nodes;
            break;

        case FDS_ProtocolInterface::FDSP_PLATFORM:
            nodes = dc_pm_nodes;
            break;

        default:
            nodes = dc_om_nodes;
            break;
    }
    fds_verify(nodes != NULL);
    return nodes;
}

// dc_register_node
// ----------------
//
Error
DomainContainer::dc_register_node(const NodeUuid       &uuid,
                                  const FdspNodeRegPtr  msg,
                                  NodeAgent::pointer   *agent)
{
    AgentContainer::pointer nodes;

    LOGDEBUG << "Domain register uuid " << std::hex
        << uuid.uuid_get_val() << ", svc uuid " << msg->service_uuid.uuid
        << ", node type " << std::dec << msg->node_type;

    nodes = dc_container_frm_msg(msg->node_type);
    // TODO(Andrew): TOTAL HACK! This sleep prevents a race
    // where the node's control interface isn't initialized
    // yet and we try and connect too early. The real fix
    // should not register until its control interface is
    // fully initialized.
    sleep(2);
    return nodes->agent_register(uuid, msg, agent, true);
}

void
DomainContainer::dc_register_node(const ShmObjRO     *shm,
                                  NodeAgent::pointer *agent,
                                  int ro, int rw, fds_uint32_t mask)
{
    const node_data_t       *node;
    NodeAgent::pointer       tmp;
    AgentContainer::pointer  container;

    fds_verify(ro != -1);
    node = shm->shm_get_rec<node_data_t>(ro);

    LOGDEBUG << "Platform domain register node uuid " << std::hex
        << node->nd_node_uuid << ", svc uuid " << node->nd_service_uuid
        << ", svc mask " << mask;

    container = dc_container_frm_msg(node->nd_svc_type);
    container->agent_register(shm, agent, ro, rw);
    if (node->nd_svc_type == fpi::FDSP_ORCH_MGR) {
        return;
    }
    fds_verify(node->nd_svc_type == fpi::FDSP_PLATFORM);

    if ((mask & fpi::NODE_SVC_SM) != 0) {
        tmp = NULL;
        dc_sm_nodes->agent_register(shm, &tmp, ro, rw);
    }
    if ((mask & fpi::NODE_SVC_DM) != 0) {
        tmp = NULL;
        dc_dm_nodes->agent_register(shm, &tmp, ro, rw);
    }
    if ((mask & fpi::NODE_SVC_AM) != 0) {
        tmp = NULL;
        dc_am_nodes->agent_register(shm, &tmp, ro, rw);
    }
    if ((mask & fpi::NODE_SVC_OM) != 0) {
        tmp = NULL;
        dc_om_nodes->agent_register(shm, &tmp, ro, rw);
    }
}

// dc_unregister_node
// ------------------
//
Error
DomainContainer::dc_unregister_node(const NodeUuid &uuid, const std::string &name)
{
    AgentContainer::pointer nodes;
    NodeAgent::pointer      agent;
    FdspNodeType            svc[] = {
        fpi::FDSP_STOR_MGR,
        fpi::FDSP_DATA_MGR,
        fpi::FDSP_STOR_HVISOR,
        fpi::FDSP_ORCH_MGR
    };

    for (int i = 0; svc[i] != FDS_ProtocolInterface::FDSP_ORCH_MGR; i++) {
        nodes = dc_container_frm_msg(svc[i]);
        if (nodes == NULL) {
            continue;
        }
        nodes->agent_unregister(uuid, name);
    }
    return Error(ERR_OK);
}

// dc_unregister_agent
// -------------------
//
Error
DomainContainer::dc_unregister_agent(const NodeUuid &uuid, FdspNodeType type)
{
    AgentContainer::pointer nodes;
    NodeAgent::pointer      agent;

    nodes = dc_container_frm_msg(type);
    if (nodes != NULL) {
        agent = nodes->agent_info(uuid);
        if (agent != NULL) {
            nodes->agent_deactivate(agent);
            return Error(ERR_OK);
        }
    }
    return Error(ERR_NOT_FOUND);
}

// dc_find_node_agent
// ------------------
//
NodeAgent::pointer
DomainContainer::dc_find_node_agent(const NodeUuid &uuid)
{
    AgentContainer::pointer nodes;
    NodeAgent::pointer      agent;
    fpi::FDSP_MgrIdType     type;

    type  = uuid.uuid_get_type();
    nodes = dc_container_frm_msg(type);
    if (nodes != NULL) {
        return nodes->agent_info(uuid);
    }
    return NULL;
}

class NodeSvcIter : public ResourceIter
{
  public:
    explicit NodeSvcIter(fpi::DomainNodes &ret) : iter_ret(ret) {}

    bool rs_iter_fn(Resource::pointer curr)
    {
        PmAgent::pointer pm;
        fpi::NodeSvcInfo info;

        pm = agt_cast_ptr<PmAgent>(curr);
        pm->agent_svc_info(&info);
        iter_ret.dom_nodes.push_back(info);
        return true;
    }

  protected:
    fpi::DomainNodes        &iter_ret;
};

// dc_node_svc_info
// ----------------
//
void
DomainContainer::dc_node_svc_info(fpi::DomainNodes &ret)
{
    NodeSvcIter iter(ret);
    dc_foreach_pm(&iter);
}

}  // namespace fds

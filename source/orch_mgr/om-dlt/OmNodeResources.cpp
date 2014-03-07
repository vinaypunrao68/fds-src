/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <vector>
#include <string>
#include <fds_err.h>
#include <OmVolume.h>
#include <OmResources.h>
#include <OmConstants.h>
#include <OmAdminCtrl.h>
#include <lib/PerfStats.h>
#include <orchMgr.h>

namespace fds {

// ---------------------------------------------------------------------------------
// OM SM NodeAgent
// ---------------------------------------------------------------------------------
OM_SmAgent::~OM_SmAgent() {}
OM_SmAgent::OM_SmAgent(const NodeUuid &uuid) : NodeAgent(uuid) {}

int
OM_SmAgent::node_calc_stor_weight()
{
    return 0;
}

// setCpSession
// ------------
//
void
OM_SmAgent::setCpSession(NodeAgentCpSessionPtr session, fpi::FDSP_MgrIdType myId)
{
    ndCpSession = session;
    ndSessionId = ndCpSession->getSessionId();
    ndCpClient  = ndCpSession->getClient();
    ndMyServId  = myId;

    FDS_PLOG_SEV(g_fdslog, fds_log::normal) << "Established connection with new node";
}

// om_send_myinfo
// --------------
// Send a node event taken from the new node agent to a peer agent.
//
void
OM_SmAgent::om_send_myinfo(NodeAgent::pointer peer)
{
    // TODO(Andrew): Add this back when OM actually responds
    // to node registrations
    // if (peer->get_node_name() == get_node_name()) {
    // return;
    // }
    fpi::FDSP_MsgHdrTypePtr     m_hdr(new fpi::FDSP_MsgHdrType);
    fpi::FDSP_Node_Info_TypePtr n_inf(new fpi::FDSP_Node_Info_Type);

    this->init_msg_hdr(m_hdr);
    this->init_node_info_pkt(n_inf);

    m_hdr->msg_code        = fpi::FDSP_MSG_NOTIFY_NODE_ADD;
    m_hdr->msg_id          = 0;
    m_hdr->tennant_id      = 1;
    m_hdr->local_domain_id = 1;

    if (node_state() == fpi::FDS_Node_Down) {
        OM_SmAgent::agt_cast_ptr(peer)->ndCpClient->NotifyNodeRmv(m_hdr, n_inf);
    } else {
        n_inf->node_state = fpi::FDS_Node_Up;
        OM_SmAgent::agt_cast_ptr(peer)->ndCpClient->NotifyNodeAdd(m_hdr, n_inf);
    }
    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
        << "Send node info from " << get_node_name()
        << " to " << peer->get_node_name() << std::endl;
}

// om_send_node_cmd
// ----------------
// TODO(Vy): define messages in inheritance tree.
//
void
OM_SmAgent::om_send_node_cmd(const om_node_msg_t &msg)
{
    const char              *log;
    fpi::FDSP_MsgHdrTypePtr  m_hdr(new fpi::FDSP_MsgHdrType);

    this->init_msg_hdr(m_hdr);
    m_hdr->msg_code = msg.nd_msg_code;

    log = NULL;
    switch (msg.nd_msg_code) {
        case fpi::FDSP_MSG_SET_THROTTLE: {
            log = "Send throttle command to node ";
            ndCpClient->SetThrottleLevel(m_hdr, *msg.u.nd_throttle);
            break;
        }
        case fpi::FDSP_MSG_DMT_UPDATE: {
            log = "Send DMT update command to node ";
            ndCpClient->NotifyDMTUpdate(m_hdr, *msg.u.nd_dmt_tab);
            break;
        }
        default:
            fds_panic("Unsupported command code");
    }
    fds_assert(log != NULL);
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << log << get_node_name() << std::endl;
}

// om_send_vol_cmd
// ---------------
// TODO(Vy): have 2 separate APIs, 1 to format the packet and 1 to send it.
//
void
OM_SmAgent::om_send_vol_cmd(VolumeInfo::pointer vol, fpi::FDSP_MsgCodeType cmd_type)
{
    om_send_vol_cmd(vol, NULL, cmd_type);
}

void
OM_SmAgent::om_send_vol_cmd(VolumeInfo::pointer    vol,
                            std::string           *vname,
                            fpi::FDSP_MsgCodeType  cmd_type)
{
    const char                *log;
    const VolumeDesc          *desc;
    fpi::FDSP_MsgHdrTypePtr    m_hdr(new fpi::FDSP_MsgHdrType);

    desc = NULL;
    this->init_msg_hdr(m_hdr);
    if (vol != NULL) {
        desc                  = vol->vol_get_properties();
        m_hdr->glob_volume_id = desc->volUUID;
    }
    switch (cmd_type) {
        case fpi::FDSP_MSG_DELETE_VOL:
        case fpi::FDSP_MSG_MODIFY_VOL:
        case fpi::FDSP_MSG_CREATE_VOL: {
            FdspNotVolPtr notif(new fpi::FDSP_NotifyVolType);

            fds_verify(vol != NULL);
            vol->vol_fmt_desc_pkt(&notif->vol_desc);
            notif->vol_name = vol->vol_get_name();
            m_hdr->msg_code = cmd_type;

            if (cmd_type == fpi::FDSP_MSG_CREATE_VOL) {
                log = "Send notify add volume ";
                notif->type = fpi::FDSP_NOTIFY_ADD_VOL;
                ndCpClient->NotifyAddVol(m_hdr, notif);

            } else if (cmd_type == fpi::FDSP_MSG_MODIFY_VOL) {
                log = "Send modify volume ";
                notif->type = fpi::FDSP_NOTIFY_MOD_VOL;
                ndCpClient->NotifyModVol(m_hdr, notif);

            } else {
                log = "Send remove volume ";
                notif->type = fpi::FDSP_NOTIFY_RM_VOL;
                ndCpClient->NotifyRmVol(m_hdr, notif);
            }
            break;
        }
        case fpi::FDSP_MSG_ATTACH_VOL_CTRL:
        case fpi::FDSP_MSG_DETACH_VOL_CTRL: {
            FdspAttVolPtr attach(new fpi::FDSP_AttachVolType);

            if (vol != NULL) {
                vol->vol_fmt_desc_pkt(&attach->vol_desc);
                attach->vol_name = vol->vol_get_name();
            } else {
                m_hdr->result    = FDSP_ERR_VOLUME_DOES_NOT_EXIST;
                m_hdr->err_msg   = "Bucket does not exist";
                attach->vol_name = *vname;
                attach->vol_desc.vol_name  = *vname;
                attach->vol_desc.volUUID   = 9876;
                attach->vol_desc.tennantId = 0;
                attach->vol_desc.localDomainId = 0;
                attach->vol_desc.capacity = 1000000;
                attach->vol_desc.volType  = FDS_ProtocolInterface::FDSP_VOL_S3_TYPE;
            }
            m_hdr->msg_code = cmd_type;

            if (cmd_type == fpi::FDSP_MSG_ATTACH_VOL_CTRL) {
                log = "Send attach volume ";
                ndCpClient->AttachVol(m_hdr, attach);
            } else {
                log = "Send detach volume ";
                ndCpClient->DetachVol(m_hdr, attach);
            }
            break;
        }
        default: {
            fds_panic("Unknown vol cmd type");
        }
    }
    if (desc != NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::normal)
                << log << desc->volUUID << " " << desc->name
                << " to node " << get_node_name();
    } else {
        FDS_PLOG_SEV(g_fdslog, fds_log::normal)
                << log << ", no vol to node " << get_node_name();
    }
}

// om_send_reg_resp
// ----------------
//
void
OM_SmAgent::om_send_reg_resp(const Error &err)
{
    fpi::FDSP_MsgHdrTypePtr       m_hdr(new fpi::FDSP_MsgHdrType);
    fpi::FDSP_RegisterNodeTypePtr r_msg(new fpi::FDSP_RegisterNodeType);

    this->init_msg_hdr(m_hdr);
    m_hdr->result       = fpi::FDSP_ERR_OK;
    m_hdr->err_code     = err.GetErrno();
    m_hdr->session_uuid = ndSessionId;

    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
        << "Sending registration result to node " << get_node_name() << std::endl;

    // TODO(Andrew): OM needs an interface to response to these messages.
    // ndCpClient->RegisterNodeResp(m_hdr, r_msg);
}

void
OM_SmAgent::om_send_dlt(const DLT *curDlt) {
    if (curDlt == NULL) {
        LOGNORMAL << "No current DLT to send to " << get_node_name();
        return;
    }

    fpi::FDSP_MsgHdrTypePtr    m_hdr(new fpi::FDSP_MsgHdrType);
    fpi::FDSP_DLT_Data_TypePtr d_msg(new fpi::FDSP_DLT_Data_Type());

    this->init_msg_hdr(m_hdr);

    m_hdr->msg_code        = fpi::FDSP_MSG_DLT_UPDATE;
    m_hdr->msg_id          = 0;
    m_hdr->tennant_id      = 1;
    m_hdr->local_domain_id = 1;

    d_msg->dlt_type        = true;
    // TODO(Andrew): It's not safe to unconst this object.
    // The serialization functions should all be const
    // since they do not modify the object
    const_cast<DLT*>(curDlt)->getSerialized(d_msg->dlt_data);

    ndCpClient->NotifyDLTUpdate(m_hdr, d_msg);

    curDlt->dump();
    LOGNORMAL << "OM: Send dlt info (version " << curDlt->getVersion()
              << ") to " << get_node_name() << " uuid 0x"
              << std::hex << (get_uuid()).uuid_get_val() << std::dec;
}

void
OM_SmAgent::init_msg_hdr(FDSP_MsgHdrTypePtr msgHdr) const
{
    msgHdr->minor_ver = 0;
    msgHdr->msg_id    = 1;
    msgHdr->major_ver = 0xa5;
    msgHdr->minor_ver = 0x5a;

    msgHdr->src_id = FDS_ProtocolInterface::FDSP_ORCH_MGR;
    msgHdr->dst_id = ndMyServId;

    msgHdr->num_objects = 1;
    msgHdr->frag_len    = 0;
    msgHdr->frag_num    = 0;

    msgHdr->tennant_id      = 0;
    msgHdr->local_domain_id = 0;
    msgHdr->src_node_name   = "";

    msgHdr->session_uuid = ndSessionId;

    msgHdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msgHdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
}

// ---------------------------------------------------------------------------------
// OM PM NodeAgent
// ---------------------------------------------------------------------------------
OM_PmAgent::~OM_PmAgent() {}
OM_PmAgent::OM_PmAgent(const NodeUuid &uuid) : NodeAgent(uuid) {}

void
OM_PmAgent::init_msg_hdr(FDSP_MsgHdrTypePtr msgHdr) const
{
    init_msg_hdr(msgHdr);
    msgHdr->src_id = FDS_ProtocolInterface::FDSP_ORCH_MGR;
    msgHdr->dst_id = FDS_ProtocolInterface::FDSP_PLATFORM;
    msgHdr->session_uuid = ndSessionId;
}

// setCpSession
// ------------
//
void
OM_PmAgent::setCpSession(NodeAgentCpSessionPtr session)
{
    ndCpSession = session;
    ndSessionId = ndCpSession->getSessionId();
    ndCpClient  = ndCpSession->getClient();

    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "PMAgent: Established connection with new node";
}

// ---------------------------------------------------------------------------------
// OM Platform NodeAgent Container
// ---------------------------------------------------------------------------------
OM_PmContainer::OM_PmContainer() : PmContainer(FDSP_ORCH_MGR)
{
    ctrlRspHndlr = boost::shared_ptr<OM_ControlRespHandler>(new OM_ControlRespHandler());
}


// agent_register
// --------------
//
Error
OM_PmContainer::agent_register(const NodeUuid       &uuid,
                               const FdspNodeRegPtr  msg,
                               NodeAgent::pointer   *out)
{
    Error err = AgentContainer::agent_register(uuid, msg, out);

    if (OM_NodeDomainMod::om_in_test_mode() || (err != ERR_OK)) {
        return err;
    }
    OM_PmAgent::pointer agent = OM_PmAgent::agt_cast_ptr(*out);
    NodeAgentCpSessionPtr session(
        ac_cpSessTbl->startSession<netControlPathClientSession>(
            agent->get_ip_str(),
            agent->get_ctrl_port(),
            FDSP_PLATFORM,  // TODO(Andrew): should be just a node
            1,              // just 1 channel for now...
            ctrlRspHndlr));

    fds_verify(agent != NULL);
    fds_verify(session != NULL);
    agent->setCpSession(session);
    return err;
}

// ---------------------------------------------------------------------------------
// OM SM NodeAgent Container
// ---------------------------------------------------------------------------------
OM_SmContainer::OM_SmContainer() : SmContainer(FDSP_ORCH_MGR)
{
    ctrlRspHndlr = boost::shared_ptr<OM_ControlRespHandler>(new OM_ControlRespHandler());
}

OM_SmContainer::~OM_SmContainer()
{
}

// agent_activate
// --------------
//
void
OM_SmContainer::agent_activate(NodeAgent::pointer agent)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "Activate node uuid " << std::hex
            << "0x" << agent->get_uuid().uuid_get_val() << std::dec;

    rs_mtx.lock();
    rs_register_mtx(agent);
    node_up_pend.push_back(OM_SmAgent::agt_cast_ptr(agent));
    rs_mtx.unlock();
}

// agent_deactivate
// ----------------
//
void
OM_SmContainer::agent_deactivate(NodeAgent::pointer agent)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "Deactivate node uuid " << std::hex
            << "0x" << agent->get_uuid().uuid_get_val() << std::dec;

    rs_mtx.lock();
    rs_unregister_mtx(agent);
    node_down_pend.push_back(OM_SmAgent::agt_cast_ptr(agent));
    rs_mtx.unlock();
}

// om_splice_nodes_pend
// --------------------
//
void
OM_SmContainer::om_splice_nodes_pend(NodeList *addNodes, NodeList *rmNodes)
{
    rs_mtx.lock();
    addNodes->splice(addNodes->begin(), node_up_pend);
    rmNodes->splice(rmNodes->begin(), node_down_pend);
    rs_mtx.unlock();
}

// agent_register
// --------------
//
Error
OM_SmContainer::agent_register(const NodeUuid       &uuid,
                               const FdspNodeRegPtr  msg,
                               NodeAgent::pointer   *out)
{
    Error err = AgentContainer::agent_register(uuid, msg, out);

    if (OM_NodeDomainMod::om_in_test_mode() || (err != ERR_OK)) {
        return err;
    }
    OM_SmAgent::pointer agent = OM_SmAgent::agt_cast_ptr(*out);
    NodeAgentCpSessionPtr session(
            ac_cpSessTbl->startSession<netControlPathClientSession>(
                agent->get_ip_str(),
                agent->get_ctrl_port(),
                FDSP_STOR_MGR,  // TODO(Andrew): should be just a node
                1,              // just 1 channel for now...
                ctrlRspHndlr));

    fds_verify(agent != NULL);
    fds_verify(session != NULL);
    agent->setCpSession(session, fpi::FDSP_STOR_MGR);
    return err;
}

// agent_unregister
// ----------------
//
Error
OM_SmContainer::agent_unregister(const NodeUuid &uuid, const std::string &name)
{
    Error err = AgentContainer::agent_unregister(uuid, name);

    return err;
}

// --------------------------------------------------------------------------------------
// OM DM NodeAgent Container
// --------------------------------------------------------------------------------------
OM_DmContainer::OM_DmContainer() : DmContainer(FDSP_ORCH_MGR)
{
    ctrlRspHndlr = boost::shared_ptr<OM_ControlRespHandler>(new OM_ControlRespHandler());
}

// agent_register
// --------------
//
Error
OM_DmContainer::agent_register(const NodeUuid       &uuid,
                               const FdspNodeRegPtr  msg,
                               NodeAgent::pointer   *out)
{
    Error err = AgentContainer::agent_register(uuid, msg, out);

    if (OM_NodeDomainMod::om_in_test_mode() || (err != ERR_OK)) {
        return err;
    }
    OM_DmAgent::pointer agent = OM_DmAgent::agt_cast_ptr(*out);
    NodeAgentCpSessionPtr session(
            ac_cpSessTbl->startSession<netControlPathClientSession>(
                agent->get_ip_str(),
                agent->get_ctrl_port(),
                FDSP_DATA_MGR,  // TODO(Andrew): should be just a node
                1,              // just 1 channel for now...
                ctrlRspHndlr));

    fds_verify(agent != NULL);
    fds_verify(session != NULL);
    agent->setCpSession(session, fpi::FDSP_DATA_MGR);
    return err;
}

// -------------------------------------------------------------------------------------
// OM AM NodeAgent Container
// -------------------------------------------------------------------------------------
OM_AmContainer::OM_AmContainer() : AmContainer(FDSP_ORCH_MGR)
{
    ctrlRspHndlr = boost::shared_ptr<OM_ControlRespHandler>(new OM_ControlRespHandler());
}

// agent_register
// --------------
//
Error
OM_AmContainer::agent_register(const NodeUuid       &uuid,
                               const FdspNodeRegPtr  msg,
                               NodeAgent::pointer   *out)
{
    Error err = AgentContainer::agent_register(uuid, msg, out);

    if (OM_NodeDomainMod::om_in_test_mode() || (err != ERR_OK)) {
        return err;
    }
    OM_AmAgent::pointer agent = OM_AmAgent::agt_cast_ptr(*out);

    NodeAgentCpSessionPtr session(
            ac_cpSessTbl->startSession<netControlPathClientSession>(
                agent->get_ip_str(),
                agent->get_ctrl_port(),
                FDSP_STOR_HVISOR,  // TODO(Andrew): should be just a node
                1,                 // just 1 channel for now...
                ctrlRspHndlr));

    fds_verify(agent != NULL);
    fds_verify(session != NULL);
    agent->setCpSession(session, fpi::FDSP_DATA_MGR);
    return err;
}

// --------------------------------------------------------------------------------------
// OM Node Container
// --------------------------------------------------------------------------------------
OM_NodeContainer::OM_NodeContainer()
    : DomainContainer("OM-Domain",
                      NULL,
                      new OM_SmContainer(),
                      new OM_DmContainer(),
                      new OM_AmContainer(),
                      new OM_PmContainer(),
                      new OmContainer(FDSP_ORCH_MGR),
                      NULL),
    om_dmt_mtx("DMT-Mtx")
{
    om_volumes    = new VolumeContainer();
}

OM_NodeContainer::~OM_NodeContainer()
{
    delete om_admin_ctrl;
}

// om_init_domain
// --------------
//
void
OM_NodeContainer::om_init_domain()
{
    om_admin_ctrl = new FdsAdminCtrl(OrchMgr::om_stor_prefix(), g_fdslog);
    om_dmt_ver    = 0;
    om_dmt_width  = 3;  // can address 2^3  DMs.
    om_dmt_depth  = 4;  // max 4 total DM replicas.
    om_curDmt     = new FdsDmt(om_dmt_width, om_dmt_depth);

    am_stats = new PerfStats(OrchMgr::om_stor_prefix() + "OM_from_AM",
                             5 * FDS_STAT_DEFAULT_HIST_SLOTS);
    if (am_stats != NULL) {
        am_stats->enable();
    }
    /* TEMP file */
    std::string fname = std::string("stats//" +
                                    OrchMgr::om_stor_prefix() +
                                    "-example.json");
    json_file.open(fname.c_str(), std::ios::out | std::ios::app);
}

// om_update_capacity
// ------------------
//
void
OM_NodeContainer::om_add_capacity(NodeAgent::pointer node)
{
    OM_SmAgent::pointer agent;

    agent = OM_SmAgent::agt_cast_ptr(node);
    if (agent->om_agent_type() != fpi::FDSP_STOR_HVISOR) {
        om_admin_ctrl->addDiskCapacity(node->node_capability());
    }
}

// om_send_my_info_to_peer
// -----------------------
//
static void
om_send_my_info_to_peer(NodeAgent::pointer me, NodeAgent::pointer peer)
{
    OM_SmAgent::agt_cast_ptr(me)->om_send_myinfo(peer);
}

// om_bcast_new_node
// -----------------
//
void
OM_NodeContainer::om_bcast_new_node(NodeAgent::pointer node, const FdspNodeRegPtr ref)
{
    if (ref->node_type == fpi::FDSP_STOR_HVISOR) {
        return;
    }
    dc_sm_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_my_info_to_peer);
    dc_dm_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_my_info_to_peer);
    dc_am_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_my_info_to_peer);
}

// om_send_peer_info_to_me
// -----------------------
//
static void
om_send_peer_info_to_me(NodeAgent::pointer me, NodeAgent::pointer peer)
{
    OM_SmAgent::agt_cast_ptr(peer)->om_send_myinfo(me);
}

// om_update_node_list
// -------------------
//
void
OM_NodeContainer::om_update_node_list(NodeAgent::pointer node, const FdspNodeRegPtr ref)
{
    dc_sm_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_peer_info_to_me);
    dc_dm_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_peer_info_to_me);
    dc_am_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_peer_info_to_me);
}

// om_send_vol_info
// ----------------
//
static void
om_send_vol_info(NodeAgent::pointer me, VolumeInfo::pointer vol)
{
    OM_SmAgent::agt_cast_ptr(me)->om_send_vol_cmd(vol, fpi::FDSP_MSG_CREATE_VOL);
}

// om_bcast_vol_list
// -----------------
//
void
OM_NodeContainer::om_bcast_vol_list(NodeAgent::pointer node)
{
    om_volumes->vol_foreach<NodeAgent::pointer>(node, om_send_vol_info);
}

// om_send_vol_command
// -------------------
// Send the volume command to the node represented by the agent.
//
static void
om_send_vol_command(fpi::FDSP_MsgCodeType cmd_type,
                    VolumeInfo::pointer   vol,
                    NodeAgent::pointer    agent)
{
    OM_SmAgent::agt_cast_ptr(agent)->om_send_vol_cmd(vol, cmd_type);
}

// om_bcast_vol_create
// -------------------
//
void
OM_NodeContainer::om_bcast_vol_create(VolumeInfo::pointer vol)
{
    dc_sm_nodes->agent_foreach<fpi::FDSP_MsgCodeType, VolumeInfo::pointer>
        (fpi::FDSP_MSG_CREATE_VOL, vol, om_send_vol_command);

    dc_dm_nodes->agent_foreach<fpi::FDSP_MsgCodeType, VolumeInfo::pointer>
        (fpi::FDSP_MSG_CREATE_VOL, vol, om_send_vol_command);
}

// om_bcast_vol_modify
// -------------------
//
void
OM_NodeContainer::om_bcast_vol_modify(VolumeInfo::pointer vol)
{
    dc_sm_nodes->agent_foreach<fpi::FDSP_MsgCodeType, VolumeInfo::pointer>
        (fpi::FDSP_MSG_MODIFY_VOL, vol, om_send_vol_command);

    dc_dm_nodes->agent_foreach<fpi::FDSP_MsgCodeType, VolumeInfo::pointer>
        (fpi::FDSP_MSG_MODIFY_VOL, vol, om_send_vol_command);

    vol->vol_foreach_am<fpi::FDSP_MsgCodeType>
        (fpi::FDSP_MSG_MODIFY_VOL, om_send_vol_command);
}

// om_bcast_vol_delete
// -------------------
//
void
OM_NodeContainer::om_bcast_vol_delete(VolumeInfo::pointer vol)
{
    dc_sm_nodes->agent_foreach<fpi::FDSP_MsgCodeType, VolumeInfo::pointer>
        (fpi::FDSP_MSG_DELETE_VOL, vol, om_send_vol_command);

    dc_dm_nodes->agent_foreach<fpi::FDSP_MsgCodeType, VolumeInfo::pointer>
        (fpi::FDSP_MSG_DELETE_VOL, vol, om_send_vol_command);

    dc_am_nodes->agent_foreach<fpi::FDSP_MsgCodeType, VolumeInfo::pointer>
        (fpi::FDSP_MSG_DELETE_VOL, vol, om_send_vol_command);
}

// om_send_node_command
// --------------------
// Plugin to send a generic node command to all nodes.  Plugin to node iterator.
//
static void
om_send_node_command(const om_node_msg_t &msg, NodeAgent::pointer node)
{
    OM_SmAgent::agt_cast_ptr(node)->om_send_node_cmd(msg);
}

// om_send_am_node_command
// -----------------------
// Plugin to send a generic node command to all am nodes.  Plugin to volume iterator.
//
void
om_send_am_node_command(const om_node_msg_t &msg,
                        VolumeInfo::pointer  vol,
                        NodeAgent::pointer   am_node)
{
    OM_SmAgent::agt_cast_ptr(am_node)->om_send_node_cmd(msg);
}

// om_bcast_vol_tier_policy
// ------------------------
//
void
OM_NodeContainer::om_bcast_vol_tier_policy(const FDSP_TierPolicyPtr &tier)
{
}

// om_bcast_vol_tier_audit
// -----------------------
//
void
OM_NodeContainer::om_bcast_vol_tier_audit(const FDSP_TierPolicyAuditPtr &tier)
{
}

// om_bcast_throttle_lvl
// ---------------------
//
void
OM_NodeContainer::om_bcast_throttle_lvl(float throttle_level)
{
    om_node_msg_t msg;
    fpi::FDSP_ThrottleMsgTypePtr throttle(new fpi::FDSP_ThrottleMsgType);

    throttle->domain_id      = DEFAULT_LOC_DOMAIN_ID;
    throttle->throttle_level = throttle_level;

    msg.nd_msg_code   = fpi::FDSP_MSG_SET_THROTTLE;
    msg.u.nd_throttle = &throttle;

    dc_am_nodes->agent_foreach<const om_node_msg_t &>(msg, om_send_node_command);
}

// om_set_throttle_lvl
// -------------------
//
void
OM_NodeContainer::om_set_throttle_lvl(float level)
{
    om_cur_throttle_level = level;

    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
        << "Setting throttle level for local domain at " << level << std::endl;

    om_bcast_throttle_lvl(level);
}

// om_bcast_tier_policy
// --------------------
//
void
OM_NodeContainer::om_bcast_tier_policy(fpi::FDSP_TierPolicyPtr policy)
{
}

// om_bcast_tier_audit
// -------------------
//
void
OM_NodeContainer::om_bcast_tier_audit(fpi::FDSP_TierPolicyAuditPtr audit)
{
}

// om_bcast_dmt_table
// ------------------
//
void
OM_NodeContainer::om_bcast_dmt_table()
{
    om_node_msg_t         msg;
    fpi::FDSP_DMT_TypePtr dmt = om_curDmt->toFdsp();

    msg.nd_msg_code  = fpi::FDSP_MSG_DMT_UPDATE;
    msg.u.nd_dmt_tab = &dmt;
    dc_am_nodes->agent_foreach<const om_node_msg_t &>(msg, om_send_node_command);
}


// om_send_dlt
// -----------------------
//
static void
om_send_dlt(const DLT* curDlt, NodeAgent::pointer agent)
{
    OM_SmAgent::agt_cast_ptr(agent)->om_send_dlt(curDlt);
}

// om_bcast_dlt
// ------------
//
void
OM_NodeContainer::om_bcast_dlt(const DLT* curDlt)
{
    dc_sm_nodes->agent_foreach<const DLT*>(curDlt, om_send_dlt);
    dc_dm_nodes->agent_foreach<const DLT*>(curDlt, om_send_dlt);
    dc_am_nodes->agent_foreach<const DLT*>(curDlt, om_send_dlt);
}

// om_round_robin_dmt
// ------------------
//
void
OM_NodeContainer::om_round_robin_dmt()
{
    int                        dm_it, node_it, total_num_nodes, bucket_depth;
    RsArray                    dmMap;
    fds_placement_table       *table;
    std::vector<fds_nodeid_t>  node_list;

    table           = static_cast<fds_placement_table *>(om_curDmt);
    bucket_depth    = table->getMaxDepth();
    total_num_nodes = dc_dm_nodes->rs_container_snapshot(&dmMap);

    if (bucket_depth > total_num_nodes) {
        bucket_depth = total_num_nodes;
    }
    dm_it = 0;
    om_dmt_mtx.lock();
    for (fds_uint32_t i = 0; i < table->getNumBuckets(); i++) {
        node_it = dm_it;
        node_list.clear();

        if (node_it == total_num_nodes) {
            break;
        }
        for (fds_int32_t j = 0; j < bucket_depth; j++) {
            node_list.push_back(dmMap[node_it]->rs_get_uuid().uuid_get_val());

            node_it++;
            if (node_it == total_num_nodes) {
                node_it = 0;
            }
        }
        Error err = table->insert(i, node_list);
        fds_assert(err == ERR_OK);

        /*
         * Move the starting point for the list and reset it to the beginning if we've
         * looped around.
         */
        dm_it++;
        if (dm_it == total_num_nodes) {
            dm_it = 0;
        }
    }
    om_dmt_mtx.unlock();
}

}  // namespace fds

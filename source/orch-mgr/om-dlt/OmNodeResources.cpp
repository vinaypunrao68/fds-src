/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <fds_error.h>
#include <OmVolume.h>
#include <OmClusterMap.h>
#include <OmResources.h>
#include <OmConstants.h>
#include <OmAdminCtrl.h>
#include <net/RpcFunc.h>
#include <orchMgr.h>
#include <NetSession.h>
#include <OmVolumePlacement.h>
#include <orch-mgr/om-service.h>
#include <fdsp/PlatNetSvc.h>
#include <net/SvcRequestPool.h>
#include <platform/node-inv-shmem.h>

namespace fds {

// ---------------------------------------------------------------------------------
// OM SM NodeAgent
// ---------------------------------------------------------------------------------
OM_NodeAgent::~OM_NodeAgent() {}
OM_NodeAgent::OM_NodeAgent(const NodeUuid &uuid, fpi::FDSP_MgrIdType type)
    : NodeAgent(uuid)
{
    node_svc_type = type;
}

int
OM_NodeAgent::node_calc_stor_weight()
{
    return 0;
}

// setCpSession
// ------------
//
void
OM_NodeAgent::setCpSession(NodeAgentCpSessionPtr session, fpi::FDSP_MgrIdType myId)
{
    ndCpSession = session;
    ndSessionId = ndCpSession->getSessionId();
    ndCpClient  = ndCpSession->getClient();
    ndMyServId  = myId;

    LOGNORMAL << "Established connection with new node";
}

// om_send_myinfo
// --------------
// Send a node event taken from the new node agent to a peer agent.
//
void
OM_NodeAgent::om_send_myinfo(NodeAgent::pointer peer)
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

    if (nd_ctrl_eph != NULL) {
        NET_SVC_RPC_CALL(nd_ctrl_eph, nd_ctrl_rpc, NotifyNodeAdd, m_hdr, n_inf);
        return;
    }
    try {
        if (node_state() == fpi::FDS_Node_Down) {
            OM_SmAgent::agt_cast_ptr(peer)->ndCpClient->NotifyNodeRmv(m_hdr, n_inf);
        } else {
            n_inf->node_state = fpi::FDS_Node_Up;
            OM_SmAgent::agt_cast_ptr(peer)->ndCpClient->NotifyNodeAdd(m_hdr, n_inf);
        }
    } catch(const att::TTransportException& e) {
        LOGERROR << "error during network call : " << e.what();
        return;
    } catch(...) {
        LOGCRITICAL << "caught unexpected exception!!!";
        throw;
    }

    LOGNORMAL << "Send node info from " << get_node_name()
              << " to " << peer->get_node_name() << std::endl;
}

// ----------------
// TODO(Vy): define messages in inheritance tree.
//
void
OM_NodeAgent::om_send_node_throttle_lvl(fpi::FDSP_ThrottleMsgTypePtr throttle)
{
    auto req =  gSvcRequestPool->newEPSvcRequest(rs_get_uuid().toSvcUuid());
    req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyThrottle), throttle);
    req->invoke();
    LOGNORMAL << " send throttle level " << throttle->throttle_level
        << " to am node " << rs_get_uuid();
}

// om_send_vol_cmd
// ---------------
// TODO(Vy): have 2 separate APIs, 1 to format the packet and 1 to send it.
Error
OM_NodeAgent::om_send_vol_cmd(VolumeInfo::pointer vol,
                              fpi::FDSPMsgTypeId      cmd_type,
                              fpi::FDSP_NotifyVolFlag vol_flag)
{
    return om_send_vol_cmd(vol, NULL, cmd_type, vol_flag);
}

void OM_NodeAgent::om_send_vol_cmd_resp(VolumeInfo::pointer     vol,
                      fpi::FDSPMsgTypeId      cmd_type,
                      EPSvcRequest* req,
                      const Error& error,
                      boost::shared_ptr<std::string> payload) {
    if (vol == NULL || vol->rs_get_uuid() == 0) {
        LOGWARN << "response received for invalid volume . ignored.";
        return;
    }
    LOGNORMAL << "received vol cmd response " << vol->vol_get_name();
    OM_NodeContainer * local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    volumes->om_vol_cmd_resp(vol, cmd_type, error, rs_get_uuid());
}

Error
OM_NodeAgent::om_send_vol_cmd(VolumeInfo::pointer     vol,
                              std::string            *vname,
                              fpi::FDSPMsgTypeId      cmd_type,
                              fpi::FDSP_NotifyVolFlag vol_flag)
{
    const char       *log;
    const VolumeDesc *desc;

    desc = NULL;
    if (vol != NULL) {
        desc = vol->vol_get_properties();
    }
    auto req =  gSvcRequestPool->newEPSvcRequest(rs_get_uuid().toSvcUuid());
    switch (cmd_type) {
    case fpi::CtrlNotifyVolAddTypeId: {
        fpi::CtrlNotifyVolAddPtr pkt(new fpi::CtrlNotifyVolAdd());

        log = "Send notify add volume ";
        if (vol != NULL) {
            vol->vol_fmt_desc_pkt(&pkt->vol_desc);
        } else {
            /* TODO(Vy): why we need to send dummy data? */
            pkt->vol_desc.vol_name  = *vname;
            pkt->vol_desc.volUUID   = 0;
            pkt->vol_desc.tennantId = 0;
            pkt->vol_desc.localDomainId = 0;
            pkt->vol_desc.capacity = 1000;
            pkt->vol_desc.volType  = fpi::FDSP_VOL_S3_TYPE;
        }
        pkt->vol_flag = vol_flag;
        req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyVolAdd), pkt);
        break;
    }
    case fpi::CtrlNotifySnapVolTypeId: {
        fpi::CtrlNotifySnapVolPtr pkt(new fpi::CtrlNotifySnapVol());

        fds_assert(vol != NULL);
        log = "Send snap volume ";
        vol->vol_fmt_desc_pkt(&pkt->vol_desc);
        pkt->vol_flag = vol_flag;
        req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifySnapVol), pkt);
        break;
    }
    case fpi::CtrlNotifyVolRemoveTypeId: {
        fpi::CtrlNotifyVolRemovePtr pkt(new fpi::CtrlNotifyVolRemove());

        fds_assert(vol != NULL);
        log = "Send remove volume ";
        vol->vol_fmt_desc_pkt(&pkt->vol_desc);
        pkt->vol_flag = vol_flag;
        req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyVolRemove), pkt);
        break;
    }
    case fpi::CtrlNotifyVolModTypeId: {
        fpi::CtrlNotifyVolModPtr pkt(new fpi::CtrlNotifyVolMod());

        fds_assert(vol != NULL);
        log = "Send modify volume ";
        vol->vol_fmt_desc_pkt(&pkt->vol_desc);
        pkt->vol_flag = vol_flag;
        req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyVolMod), pkt);
        break;
    }
    default:
        fds_panic("Unknown vol cmd type");
    }
    if (!vol) {
        vol = VolumeInfo::pointer(new VolumeInfo(0));  // create dummy to avoid issues
    }
    EPSvcRequestRespCb cb = std::bind(&OM_NodeAgent::om_send_vol_cmd_resp, this, vol, cmd_type,
                                   std::placeholders::_1, std::placeholders::_2,
                                   std::placeholders::_3);
    req->onResponseCb(cb);
    req->invoke();
    if (desc != NULL) {
        Platform *plat = Platform::platf_singleton();
        int ctrl_port = plat->plf_get_my_ctrl_port(node_base_port());
        LOGNORMAL << log << desc->volUUID << " " << desc->name
                  << " to node " << get_node_name() << std::hex
                  << ", uuid " << get_uuid().uuid_get_val() << std::dec
                  << ", port " << ctrl_port;
    } else {
        LOGNORMAL << log << ", no vol to node " << get_node_name();
    }
    return Error(ERR_OK);
}


// om_send_reg_resp
// ----------------
//
void
OM_NodeAgent::om_send_reg_resp(const Error &err)
{
    fpi::FDSP_MsgHdrTypePtr       m_hdr(new fpi::FDSP_MsgHdrType);
    fpi::FDSP_RegisterNodeTypePtr r_msg(new fpi::FDSP_RegisterNodeType);

    this->init_msg_hdr(m_hdr);
    m_hdr->result       = fpi::FDSP_ERR_OK;
    m_hdr->err_code     = err.GetErrno();
    m_hdr->session_uuid = ndSessionId;

    LOGNORMAL << "Sending registration result to node " << get_node_name() << std::endl;

    // TODO(Andrew): OM needs an interface to response to these messages.
    // ndCpClient->RegisterNodeResp(m_hdr, r_msg);
}

Error
OM_NodeAgent::om_send_dlt(const DLT *curDlt) {
    Error err(ERR_OK);
    if (curDlt == NULL) {
        LOGNORMAL << "No current DLT to send to " << get_node_name();
        return Error(ERR_NOT_FOUND);
    }

    auto om_req =  gSvcRequestPool->newEPSvcRequest(rs_get_uuid().toSvcUuid());
    fpi::CtrlNotifyDLTUpdatePtr msg(new fpi::CtrlNotifyDLTUpdate());
    auto d_msg = &msg->dlt_data;

    d_msg->dlt_type        = true;
    err = const_cast<DLT*>(curDlt)->getSerialized(d_msg->dlt_data);
    msg->dlt_version = curDlt->getVersion();
    if (!err.ok()) {
        LOGERROR << "Failed to fill in dlt_data, not sending DLT";
        return err;
    }
    om_req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyDLTUpdate), msg);
    om_req->onResponseCb(std::bind(&OM_NodeAgent::om_send_dlt_resp, this, msg,
                                   std::placeholders::_1, std::placeholders::_2,
                                   std::placeholders::_3));
    om_req->setTimeoutMs(20000);  // huge, but need to handle timeouts in resp
    om_req->invoke();

    curDlt->dump();
    LOGNORMAL << "OM: Send dlt info (version " << curDlt->getVersion()
              << ") to " << get_node_name() << " uuid 0x"
              << std::hex << (get_uuid()).uuid_get_val() << std::dec;

    return err;
}

Error
OM_NodeAgent::om_send_dlt_close(fds_uint64_t cur_dlt_version) {
    Error err(ERR_OK);

    auto om_req = gSvcRequestPool->newEPSvcRequest(rs_get_uuid().toSvcUuid());
    fpi::CtrlNotifyDLTClosePtr msg(new fpi::CtrlNotifyDLTClose());
    msg->dlt_close.DLT_version = cur_dlt_version;

    om_req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyDLTClose), msg);
    om_req->onResponseCb(std::bind(&OM_NodeAgent::om_send_dlt_close_resp, this, msg,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    om_req->setTimeoutMs(5000);
    om_req->invoke();

    LOGNORMAL << "OM: send dlt close (version " << cur_dlt_version
                << ") to " << get_node_name() << " uuid 0x"
                << std::hex << (get_uuid()).uuid_get_val() << std::dec;

    return err;
}

void
OM_NodeAgent::om_send_dlt_close_resp(fpi::CtrlNotifyDLTClosePtr msg,
        EPSvcRequest* req,
        const Error& error,
        boost::shared_ptr<std::string> payload)
{
    LOGDEBUG << "OM received response for NotifyDltClose from node "
                << std::hex << req->getPeerEpId().svc_uuid << std::dec <<
                " with version " << msg->dlt_close.DLT_version;
}

void
OM_NodeAgent::om_send_dlt_resp(fpi::CtrlNotifyDLTUpdatePtr msg, EPSvcRequest* req,
                               const Error& error,
                               boost::shared_ptr<std::string> payload)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "OM received response for NotifyDltUpdate from node "
            << std::hex << req->getPeerEpId().svc_uuid << std::dec <<
            " with version " << msg->dlt_version;

    // notify DLT state machine
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    NodeUuid node_uuid(rs_get_uuid());
    FdspNodeType node_type = rs_get_uuid().uuid_get_type();
    domain->om_recv_dlt_commit_resp(node_type, node_uuid, msg->dlt_version);
}

Error
OM_NodeAgent::om_send_dmt_x(const DMTPtr& curDmt) {
    Error err(ERR_OK);
    fds_verify(curDmt->getVersion() != DMT_VER_INVALID);
    auto om_req =  gSvcRequestPool->newEPSvcRequest(rs_get_uuid().toSvcUuid());
    fpi::CtrlNotifyDMTUpdatePtr msg(new fpi::CtrlNotifyDMTUpdate());
    auto dmt_msg = &msg->dmt_data;
    dmt_msg->dmt_version = curDmt->getVersion();
    err = curDmt->getSerialized(dmt_msg->dmt_data);
    if (!err.ok()) {
        LOGERROR << "Failed to fill in dmt_data, not sending DMT";
        return err;
    }
    om_req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyDMTUpdate), msg);
    om_req->onResponseCb(std::bind(&OM_NodeAgent::om_send_dmt_x_resp, this, msg,
                                   std::placeholders::_1, std::placeholders::_2,
                                   std::placeholders::_3));
    om_req->invoke();
    LOGNORMAL << "OM: Send dmt info (version " << curDmt->getVersion()
              << ") to " << get_node_name() << " uuid 0x"
              << std::hex << (get_uuid()).uuid_get_val() << std::dec;
    return err;
}

void
OM_NodeAgent::om_send_dmt_x_resp(fpi::CtrlNotifyDMTUpdatePtr msg, EPSvcRequest* req,
                               const Error& error,
                               boost::shared_ptr<std::string> payload)
{
}


Error
OM_NodeAgent::om_send_dmt(const DMTPtr& curDmt) {
    Error err(ERR_OK);
    fds_verify(curDmt->getVersion() != DMT_VER_INVALID);

    fpi::FDSP_MsgHdrTypePtr    m_hdr(new fpi::FDSP_MsgHdrType);
    this->init_msg_hdr(m_hdr);
    m_hdr->msg_code        = fpi::FDSP_MSG_DMT_UPDATE;
    m_hdr->msg_id          = 0;
    m_hdr->tennant_id      = 1;
    m_hdr->local_domain_id = 1;

    fpi::FDSP_DMT_TypePtr dmt_msg(new fpi::FDSP_DMT_Type());
    dmt_msg->dmt_version = curDmt->getVersion();
    err = curDmt->getSerialized(dmt_msg->dmt_data);
    if (!err.ok()) {
        LOGERROR << "Failed to fill in dmt_data, not sending DMT";
        return err;
    }
    if (nd_ctrl_eph != NULL) {
        NET_SVC_RPC_CALL(nd_ctrl_eph, nd_ctrl_rpc, NotifyDMTUpdate, m_hdr, dmt_msg);
    } else {
        try {
            ndCpClient->NotifyDMTUpdate(m_hdr, dmt_msg);
        } catch(const att::TTransportException& e) {
            LOGERROR << "error during network call : " << e.what();
            return Error(ERR_NETWORK_TRANSPORT);
        }
    }
    LOGNORMAL << "OM: Send dmt info (version " << curDmt->getVersion()
              << ") to " << get_node_name() << " uuid 0x"
              << std::hex << (get_uuid()).uuid_get_val() << std::dec;

    return err;
}

//
// Currently sends scavenger start message
// TODO(xxx) extend to other scavenger commands (pass cmd type)
//
Error
OM_NodeAgent::om_send_scavenger_cmd(fpi::FDSP_ScavengerCmd cmd) {
    fpi::CtrlNotifyScavengerPtr msg(new fpi::CtrlNotifyScavenger());
    fpi::FDSP_ScavengerType *gc_msg = &msg->scavenger;
    gc_msg->cmd = cmd;
    auto req =  gSvcRequestPool->newEPSvcRequest(rs_get_uuid().toSvcUuid());
    req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyScavenger), msg);
    req->invoke();
    LOGNORMAL << "OM: send scavenger command: " << cmd;
    return Error(ERR_OK);
}

Error
OM_NodeAgent::om_send_qosinfo(fds_uint64_t total_rate) {
    fpi::CtrlNotifyQoSControlPtr qos_msg(new fpi::CtrlNotifyQoSControl());
    fpi::FDSP_QoSControlMsgType *qosctrl = &qos_msg->qosctrl;
    qosctrl->total_rate = total_rate;
    auto req =  gSvcRequestPool->newEPSvcRequest(rs_get_uuid().toSvcUuid());
    req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyQoSControl), qos_msg);
    req->invoke();
    LOGNORMAL << "OM: send total rate to AM: " << total_rate;
    return Error(ERR_OK);
}

Error
OM_NodeAgent::om_send_stream_reg_cmd(fds_int32_t regId,
                                     fds_bool_t bAll) {
    Error err(ERR_OK);

    // get UUID of any AM
    OM_NodeContainer* local = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_AmContainer::pointer amNodes = local->om_am_nodes();
    NodeAgent::pointer agent = amNodes->agent_info(0);
    if (!agent) {
        LOGERROR << "There are no AMs, cannot broadcast stream registration "
                 << " to DMs, will do when at least one AM joins";
        return ERR_NOT_READY;
    }
    NodeUuid am_uuid = agent->get_uuid();

    kvstore::ConfigDB* configDB = gl_orch_mgr->getConfigDB();
    if (bAll == true) {
        // send all known registrations
        std::vector<fpi::StreamingRegistrationMsg> reg_vec;
        fds_bool_t bret = configDB->getStreamRegistrations(reg_vec);
        if (!bret) {
            LOGERROR << "Failed to get stream registrations from configDB";
            return ERR_NOT_READY;
        }
        LOGDEBUG << "Got " << reg_vec.size() << " stream registrations "
                 << " from configDB";
        for (fds_uint32_t i = 0; i < reg_vec.size(); ++i) {
            om_send_one_stream_reg_cmd(reg_vec[i], am_uuid);
        }
    } else {
        fpi::StreamingRegistrationMsg reg_msg;
        fds_bool_t bret = configDB->getStreamRegistration(regId, reg_msg);
        if (!bret) {
            LOGERROR << "Failed to get stream registration "
                     << regId << " from configDB";
            return ERR_NOT_READY;
        }
        om_send_one_stream_reg_cmd(reg_msg, am_uuid);
    }

    return err;
}

void
OM_NodeAgent::om_send_one_stream_reg_cmd(const fpi::StreamingRegistrationMsg& reg,
                                         const NodeUuid& stream_dest_uuid) {
    fpi::StatStreamRegistrationMsgPtr reg_msg(new fpi::StatStreamRegistrationMsg());
    reg_msg->id = reg.id;
    reg_msg->url = reg.url;
    reg_msg->method = reg.http_method;
    (reg_msg->dest).svc_uuid = stream_dest_uuid.uuid_get_val();
    reg_msg->sample_freq_seconds = reg.sample_freq_seconds;
    reg_msg->duration_seconds = reg.duration_seconds;

    (reg_msg->volumes).reserve((reg.volume_names).size());
    for (uint i = 0; i < (reg.volume_names).size(); i++) {
        std::string volname = (reg.volume_names).at(i);
        OM_NodeContainer * local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volumes = local->om_vol_mgr();
        VolumeInfo::pointer vol = volumes->get_volume(volname);
        (reg_msg->volumes).push_back(vol->rs_get_uuid().uuid_get_val());
    }

    LOGDEBUG << "Will send StatStreamRegistration with id " << reg.id
             << "to DM " << std::hex << rs_uuid.uuid_get_val() << ", AM uuid is "
             << std::hex << stream_dest_uuid.uuid_get_val() << std::dec;

    auto asyncStreamRegReq = gSvcRequestPool->newEPSvcRequest(rs_uuid.toSvcUuid());
    asyncStreamRegReq->setPayload(FDSP_MSG_TYPEID(fpi::StatStreamRegistrationMsg), reg_msg);
    // HACK
    asyncStreamRegReq->setTimeoutMs(0);
    asyncStreamRegReq->invoke();
}

#if 0
Error
OM_NodeAgent::om_send_dlt_close(fds_uint64_t cur_dlt_version) {
    Error err(ERR_OK);
    fpi::FDSP_MsgHdrTypePtr m_hdr(new fpi::FDSP_MsgHdrType);
    fpi::FDSP_DltCloseTypePtr d_msg(new fpi::FDSP_DltCloseType());
    this->init_msg_hdr(m_hdr);

    m_hdr->msg_code = fpi::FDSP_MSG_DLT_CLOSE;
    m_hdr->msg_id = 0;
    m_hdr->tennant_id = 1;
    m_hdr->local_domain_id = 1;

    d_msg->DLT_version = cur_dlt_version;
    if (nd_ctrl_eph != NULL) {
        NET_SVC_RPC_CALL(nd_ctrl_eph, nd_ctrl_rpc, NotifyDLTClose, m_hdr, d_msg);
    } else {
        try {
            ndCpClient->NotifyDLTClose(m_hdr, d_msg);
        } catch(const att::TTransportException& e) {
            LOGERROR << "error during network call : " << e.what();
            return Error(ERR_NETWORK_TRANSPORT);
        }
    }
    LOGNORMAL << "OM: send dlt close (version " << cur_dlt_version
              << ") to " << get_node_name() << " uuid 0x"
              << std::hex << (get_uuid()).uuid_get_val() << std::dec;

    return err;
}
#endif


Error
OM_NodeAgent::om_send_pushmeta(fpi::FDSP_PushMetaPtr& meta_msg)
{
    Error err(ERR_OK);
    fpi::FDSP_MsgHdrTypePtr m_hdr(new fpi::FDSP_MsgHdrType);
    this->init_msg_hdr(m_hdr);

    m_hdr->msg_code = fpi::FDSP_MSG_PUSH_META;
    m_hdr->msg_id = 0;
    m_hdr->tennant_id = 1;
    m_hdr->local_domain_id = 1;

    if (nd_ctrl_eph != NULL) {
        NET_SVC_RPC_CALL(nd_ctrl_eph, nd_ctrl_rpc, PushMetaDMTReq, m_hdr, meta_msg);
    } else {
        try {
            ndCpClient->PushMetaDMTReq(m_hdr, meta_msg);
        } catch(const att::TTransportException& e) {
            LOGERROR << "error during network call : " << e.what();
            return Error(ERR_NETWORK_TRANSPORT);
        }
    }
    LOGNORMAL << "OM: send Push_Meta to " << get_node_name() << " uuid 0x"
              << std::hex << (get_uuid()).uuid_get_val() << std::dec;

    return err;
}

Error
OM_NodeAgent::om_send_dmt_close(fds_uint64_t dmt_version) {
    Error err(ERR_OK);
    fpi::FDSP_MsgHdrTypePtr m_hdr(new fpi::FDSP_MsgHdrType);
    fpi::FDSP_DmtCloseTypePtr d_msg(new fpi::FDSP_DmtCloseType());
    this->init_msg_hdr(m_hdr);

    m_hdr->msg_code = fpi::FDSP_MSG_DMT_CLOSE;
    m_hdr->msg_id = 0;
    m_hdr->tennant_id = 1;
    m_hdr->local_domain_id = 1;

    d_msg->DMT_version = dmt_version;
    if (nd_ctrl_eph != NULL) {
        NET_SVC_RPC_CALL(nd_ctrl_eph, nd_ctrl_rpc, NotifyDMTClose, m_hdr, d_msg);
    } else {
        try {
            ndCpClient->NotifyDMTClose(m_hdr, d_msg);
        } catch(const att::TTransportException& e) {
            LOGERROR << "error during network call : " << e.what();
            return Error(ERR_NETWORK_TRANSPORT);
        }
    }
    LOGNORMAL << "OM: send DMT close (version " << dmt_version
              << ") to " << get_node_name() << " uuid 0x"
              << std::hex << (get_uuid()).uuid_get_val() << std::dec;

    return err;
}


void
OM_NodeAgent::init_msg_hdr(FDSP_MsgHdrTypePtr msgHdr) const
{
    NodeInventory::init_msg_hdr(msgHdr);

    msgHdr->src_id       = FDS_ProtocolInterface::FDSP_ORCH_MGR;
    msgHdr->dst_id       = ndMyServId;
    msgHdr->session_uuid = ndSessionId;
}

// ---------------------------------------------------------------------------------
// OM PM NodeAgent
// ---------------------------------------------------------------------------------
OM_PmAgent::~OM_PmAgent() {}
OM_PmAgent::OM_PmAgent(const NodeUuid &uuid)
    : OM_NodeAgent(uuid, fpi::FDSP_PLATFORM) {}

void
OM_PmAgent::init_msg_hdr(FDSP_MsgHdrTypePtr msgHdr) const
{
    NodeInventory::init_msg_hdr(msgHdr);
    msgHdr->src_id = FDS_ProtocolInterface::FDSP_ORCH_MGR;
    msgHdr->dst_id = FDS_ProtocolInterface::FDSP_PLATFORM;
    msgHdr->session_uuid = ndSessionId;
}


// service_exists
// --------------
//
fds_bool_t
OM_PmAgent::service_exists(FDS_ProtocolInterface::FDSP_MgrIdType svc_type) const
{
    switch (svc_type) {
        case FDS_ProtocolInterface::FDSP_STOR_MGR:
            if (activeSmAgent != NULL)
                return true;
            break;
        case FDS_ProtocolInterface::FDSP_DATA_MGR:
            if (activeDmAgent != NULL)
                return true;
            break;
        case FDS_ProtocolInterface::FDSP_STOR_HVISOR:
            if (activeAmAgent != NULL)
                return true;
            break;
        default:
            break;
    };
    return false;
}

// register_service
// ----------------
//
Error
OM_PmAgent::handle_register_service(FDS_ProtocolInterface::FDSP_MgrIdType svc_type,
                                    NodeAgent::pointer svc_agent)
{
    // we cannot register more than one service of the same type
    // with the same node (platform)
    if (service_exists(svc_type)) {
        return Error(ERR_DUPLICATE);
    }

    // update configDB with which services this platform has
    kvstore::ConfigDB* configDB = gl_orch_mgr->getConfigDB();
    NodeServices services;
    if (configDB && !configDB->getNodeServices(get_uuid(), services)) {
        // just in case reset services to 0
        services.reset();
    }

    switch (svc_type) {
        case FDS_ProtocolInterface::FDSP_STOR_MGR:
            activeSmAgent = OM_SmAgent::agt_cast_ptr(svc_agent);
            services.sm = svc_agent->get_uuid();
            LOGDEBUG << " sm: " <<  std::hex << services.sm.uuid_get_val() << std::dec;
            break;
        case FDS_ProtocolInterface::FDSP_DATA_MGR:
            activeDmAgent = OM_DmAgent::agt_cast_ptr(svc_agent);
            services.dm = svc_agent->get_uuid();
            LOGDEBUG << " dm: " <<  std::hex << services.dm.uuid_get_val() << std::dec;
            break;
        case FDS_ProtocolInterface::FDSP_STOR_HVISOR:
            activeAmAgent = OM_AmAgent::agt_cast_ptr(svc_agent);
            services.am = svc_agent->get_uuid();
            LOGDEBUG << " am: " <<  std::hex << services.am.uuid_get_val() << std::dec;
            break;
        default:
            fds_verify(false);
    };

    // actually update config DB
    if (configDB) {
        configDB->setNodeServices(get_uuid(), services);
    }

    return Error(ERR_OK);
}

// unregister_service
// ------------------
//
NodeUuid
OM_PmAgent::handle_unregister_service(FDS_ProtocolInterface::FDSP_MgrIdType svc_type)
{
    // update configDB -- remove the service
    kvstore::ConfigDB* configDB = gl_orch_mgr->getConfigDB();
    NodeServices services;
    fds_bool_t found_services = configDB ?
            configDB->getNodeServices(get_uuid(), services) : false;

    NodeUuid svc_uuid;
    switch (svc_type) {
        case FDS_ProtocolInterface::FDSP_STOR_MGR:
            activeSmAgent = NULL;
            svc_uuid = services.sm;
            services.sm.uuid_set_val(0);
            break;
        case FDS_ProtocolInterface::FDSP_DATA_MGR:
            activeDmAgent = NULL;
            svc_uuid = services.dm;
            services.dm.uuid_set_val(0);
            break;
        case FDS_ProtocolInterface::FDSP_STOR_HVISOR:
            activeAmAgent = NULL;
            svc_uuid = services.am;
            services.am.uuid_set_val(0);
            break;
        default:
            fds_verify(false);
    };

    if (found_services) {
        configDB->setNodeServices(get_uuid(), services);
    }

    return svc_uuid;
}

void
OM_PmAgent::handle_unregister_service(const NodeUuid& uuid)
{
    if (activeSmAgent->get_uuid() == uuid) {
        handle_unregister_service(FDS_ProtocolInterface::FDSP_STOR_MGR);
    } else if (activeDmAgent->get_uuid() == uuid) {
        handle_unregister_service(FDS_ProtocolInterface::FDSP_DATA_MGR);
    } else if (activeAmAgent->get_uuid() == uuid) {
        handle_unregister_service(FDS_ProtocolInterface::FDSP_STOR_HVISOR);
    }
}

// send_activate_services
// -----------------------
//
Error
OM_PmAgent::send_activate_services(fds_bool_t activate_sm,
                                   fds_bool_t activate_dm,
                                   fds_bool_t activate_am)
{
    Error err(ERR_OK);
    fds_bool_t do_activate_sm = activate_sm;
    fds_bool_t do_activate_dm = activate_dm;
    fds_bool_t do_activate_am = activate_am;

    // we only activate services from 'discovered' state or
    // 'node up' state
    if ((node_state() != FDS_ProtocolInterface::FDS_Node_Discovered) &&
        (node_state() != FDS_ProtocolInterface::FDS_Node_Up)) {
        LOGERROR << "Invalid state";
        return Error(ERR_INVALID_ARG);
    }
    LOGNORMAL << "OM_PmAgent: will send node activate message to " << get_node_name()
              << "; activate sm: " << activate_sm << "; activate dm: "<< activate_dm
              << "; activate am: " << activate_am;

    // we are ok to activate a service after we already activate another
    // services on this node, but check if the requested services are already
    // running
    if (node_state() == FDS_ProtocolInterface::FDS_Node_Up) {
        if (activate_sm && service_exists(FDS_ProtocolInterface::FDSP_STOR_MGR)) {
            LOGNOTIFY << "OM_PmAgent: SM service already running, "
                      << "not going to restart...";
            do_activate_sm = false;
        }
        if (activate_dm && service_exists(FDS_ProtocolInterface::FDSP_DATA_MGR)) {
            LOGNOTIFY << "OM_PmAgent: DM service already running, "
                      << "not going to restart...";
            do_activate_dm = false;
        }
        if (activate_am && service_exists(FDS_ProtocolInterface::FDSP_STOR_HVISOR)) {
            LOGNOTIFY << "OM_PmAgent: AM service already running. Allowing another "
                      << "AM instance...";
            // TODO(Andrew): Re-enable this if we want to prevent multiple AM
            // instances per node.
            // do_activate_am = false;
        }

        //  if all requested services already active, nothing to do
        if (!do_activate_sm && !do_activate_dm && !do_activate_am) {
            LOGNOTIFY << "All services already running, nothing to activate"
                      << " on node " << get_node_name();
            return err;
        }
    } else {
        // TODO(anna) we should set node active state when we get a response
        // for node activate + update config DB with node info
        // but for now assume always success and set active state here
        set_node_state(FDS_ProtocolInterface::FDS_Node_Up);
        kvstore::ConfigDB* configDB = gl_orch_mgr->getConfigDB();
        node_data_t node_data;
        if (!configDB->getNode(get_uuid(), &node_data)) {
            // for now store only if the node was not known to DB
            node_info_frm_shm(&node_data);
            configDB->addNode(&node_data);
            LOGNOTIFY << "Adding node info for " << get_node_name() << ":"
                      << std::hex << get_uuid().uuid_get_val() << std::dec
                      << " in configDB";
        }
    }

    fpi::FDSP_MsgHdrTypePtr    m_hdr(new fpi::FDSP_MsgHdrType);
    fpi::FDSP_ActivateNodeTypePtr node_msg(new fpi::FDSP_ActivateNodeType());

    init_msg_hdr(m_hdr);
    m_hdr->msg_code        = fpi::FDSP_MSG_NOTIFY_NODE_ACTIVE;
    m_hdr->msg_id          = 0;
    m_hdr->tennant_id      = 1;
    m_hdr->local_domain_id = 1;

    (node_msg->node_uuid).uuid = get_uuid().uuid_get_val();
    node_msg->node_name = get_node_name();
    node_msg->has_sm_service = activate_sm;
    node_msg->has_dm_service = activate_dm;
    node_msg->has_am_service = activate_am;
    node_msg->has_om_service = false;

    NET_SVC_RPC_CALL(nd_eph, nd_svc_rpc, notifyNodeActive, node_msg);
    return err;
}

// ---------------------------------------------------------------------------------
// Common OM Service Container
// ---------------------------------------------------------------------------------
OM_AgentContainer::OM_AgentContainer(FdspNodeType id) : AgentContainer(id)
{
    ctrlRspHndlr = boost::shared_ptr<OM_ControlRespHandler>(new OM_ControlRespHandler());
}

// agent_register
// --------------
//
Error
OM_AgentContainer::agent_register(const NodeUuid       &uuid,
                                  const FdspNodeRegPtr  msg,
                                  NodeAgent::pointer   *out,
                                  bool                  activate)
{
    Error err = AgentContainer::agent_register(uuid, msg, out, false);

    if (OM_NodeDomainMod::om_in_test_mode() || (err != ERR_OK)) {
        return err;
    }
    OM_NodeAgent::pointer agent = OM_NodeAgent::agt_cast_ptr(*out);

    if (agent->node_get_svc_type() == fpi::FDSP_PLATFORM) {
        agent->node_agent_up();
        agent_activate(agent);
        return err;
    }

    try {
        Platform *plat = Platform::platf_singleton();
        int ctrl_port = plat->plf_get_my_ctrl_port(agent->node_base_port());
        NodeAgentCpSessionPtr session(
                ac_cpSessTbl->startSession<netControlPathClientSession>(
                    agent->get_ip_str(),
                    ctrl_port,
                    ac_id,      // TODO(Andrew): should be just a node
                    1,                 // just 1 channel for now...
                    ctrlRspHndlr));

        fds_verify(agent != NULL);
        fds_verify(session != NULL);
        agent->setCpSession(session, fpi::FDSP_DATA_MGR);

        LOGNOTIFY << "Agent uuid " << std::hex << agent->get_uuid().uuid_get_val()
            << std::dec << " connects ip " << agent->get_ip_str()
            << ", port " << ctrl_port;
    } catch(const att::TTransportException& e) {
        rs_free_resource(agent);
        LOGERROR << "error during network call : " << e.what();
        return ERR_NETWORK_TRANSPORT;
    }
    // Only make it known to the container when we have the endpoint.
    // XXX(Vy): it's possible that we can lost the endpoint during the activate call.
    //
    agent_activate(agent);
    return err;
}

// agent_unregister
// ----------------
//
Error
OM_AgentContainer::agent_unregister(const NodeUuid &uuid, const std::string &name)
{
    Error err = AgentContainer::agent_unregister(uuid, name);

    return err;
}

// ---------------------------------------------------------------------------------
// OM Platform NodeAgent Container
// ---------------------------------------------------------------------------------
OM_PmContainer::OM_PmContainer() : OM_AgentContainer(fpi::FDSP_PLATFORM) {}

// agent_register
// --------------
//
Error
OM_PmContainer::agent_register(const NodeUuid       &uuid,
                               const FdspNodeRegPtr  msg,
                               NodeAgent::pointer   *out,
                               bool                  activate)
{
    // check if this is a known Node
    bool        known;
    node_data_t node;
    kvstore::ConfigDB* configDB = gl_orch_mgr->getConfigDB();

    if (configDB->getNode(uuid, &node)) {
        // this is a known node
        known = true;
        msg->node_name = node.nd_assign_name;
    } else {
        // we are ignoring name that platform sends us
        known = false;
        if (msg->node_name.empty() || msg->node_name == "auto") {
            uint cfgNameCounter = configDB->getNodeNameCounter();
            if (cfgNameCounter > 0) {
                nodeNameCounter = cfgNameCounter;
            } else {
                nodeNameCounter++;
            }
            msg->node_name.clear();
            char buf[20];
            snprintf(buf, sizeof(buf), "Node-%d", nodeNameCounter);
            msg->node_name.append(buf);
            LOGNORMAL << "autonamed node : " << msg->node_name;
        } else {
            LOGNOTIFY << "Using user provided name: " << msg->node_name;
        }
    }
    Error err = OM_AgentContainer::agent_register(uuid, msg, out, activate);
    if ((err == ERR_OK) && (known == true)) {
        fds_verify(out != NULL);
        OM_PmAgent::pointer agent = OM_PmAgent::agt_cast_ptr(*out);

        // start services that were running on that node
        NodeServices services;
        fds_bool_t has_am = false;
        fds_bool_t has_sm = false;
        fds_bool_t has_dm = false;
        if (configDB->getNodeServices(uuid, services)) {
            if (services.am.uuid_get_val() != 0) {
                has_am = true;
            }
            if (services.sm.uuid_get_val() != 0) {
                has_sm = true;
            }
            if (services.dm.uuid_get_val() != 0) {
                has_dm = true;
            }
        }
        LOGNORMAL << "Known node uuid " << agent->get_uuid().uuid_get_val()
                  << ", name " << agent->get_node_name() << ", start services:"
                  << " am:" << has_am << " sm:" << has_sm << " dm:" << has_dm;

        if (has_am || has_sm || has_dm) {
            agent->send_activate_services(has_sm, has_dm, has_am);
        }
    }
    return err;
}

// check_new_service
// -----------------
//
fds_bool_t
OM_PmContainer::check_new_service(const NodeUuid &pm_uuid,
                                  FDS_ProtocolInterface::FDSP_MgrIdType svc_role) {
    fds_bool_t bret = false;
    NodeAgent::pointer agent = agent_info(pm_uuid);
    if (agent == NULL) {
        LOGWARN << "agent for PM node does not exit";
        return false;  // we must have pm node
    } else if (agent->node_state() != FDS_ProtocolInterface::FDS_Node_Up) {
        // TODO(anna) for now using NodeUp state as active, review states
        LOGWARN << "PM agent not in Node_Up state";
        return false;  // must be in active state
    }

    bret = (OM_PmAgent::agt_cast_ptr(agent)->service_exists(svc_role) == false);
    LOGDEBUG << "Service of type " << svc_role << " on node " << std::hex
             << pm_uuid.uuid_get_val() << std::dec << " exists? " << bret;
    return bret;
}

// handle_register_service
// -----------------------
//
Error
OM_PmContainer::handle_register_service(const NodeUuid &pm_uuid,
                                        FDS_ProtocolInterface::FDSP_MgrIdType svc_role,
                                        NodeAgent::pointer svc_agent)
{
    Error err(ERR_OK);
    NodeAgent::pointer pm_agt = agent_info(pm_uuid);

    if (pm_agt == NULL) {
        return Error(ERR_NODE_NOT_ACTIVE);
    }
    return OM_PmAgent::agt_cast_ptr(pm_agt)->handle_register_service(svc_role, svc_agent);
}

// handle_unregister_service
// -------------------------
//
NodeUuid
OM_PmContainer::handle_unregister_service(const NodeUuid& node_uuid,
                                          const std::string& node_name,
                                          FDS_ProtocolInterface::FDSP_MgrIdType svc_type)
{
    NodeUuid svc_uuid;
    NodeAgent::pointer agent;
    if (node_uuid.uuid_get_val() == 0) {
        NodeUuid nd_uuid;
        for (fds_uint32_t i = 0; i < rs_available_elm(); ++i) {
            agent = agent_info(i);
            if (node_name.compare(agent->get_node_name()) == 0) {
                // we found node agent!
                if (nd_uuid.uuid_get_val() != 0) {
                    LOGWARN << "Found more than one node with the same name "
                            << " -- ambiguous, will not unregister service";
                    return svc_uuid;
                }
                // first node that we found
                nd_uuid = agent->get_uuid();
            }
        }
        if (nd_uuid.uuid_get_val() == 0) {
            LOGWARN << "Could not find platform agent for node " << node_name;
            return svc_uuid;
        }
        agent = agent_info(nd_uuid);
        svc_uuid = OM_PmAgent::agt_cast_ptr(
            agent)->handle_unregister_service(svc_type);
    } else {
        agent = agent_info(node_uuid);
        if (agent) {
            svc_uuid = OM_PmAgent::agt_cast_ptr(
                agent)->handle_unregister_service(svc_type);
        } else {
            LOGWARN << "Could not find platform agent for node uuid "
                    << std::hex << node_uuid.uuid_get_val() << std::dec;
        }
    }
    return svc_uuid;
}

// ---------------------------------------------------------------------------------
// OM SM NodeAgent Container
// ---------------------------------------------------------------------------------
OM_SmContainer::OM_SmContainer() : OM_AgentContainer(fpi::FDSP_STOR_MGR) {}

// agent_activate
// --------------
//
void
OM_AgentContainer::agent_activate(NodeAgent::pointer agent)
{
    LOGNORMAL << "Activate node uuid " << std::hex
              << "0x" << agent->get_uuid().uuid_get_val() << std::dec;

    rs_mtx.lock();
    rs_register_mtx(agent);
    node_up_pend.push_back(OM_NodeAgent::agt_cast_ptr(agent));
    rs_mtx.unlock();
}

// agent_deactivate
// ----------------
//
void
OM_AgentContainer::agent_deactivate(NodeAgent::pointer agent)
{
    LOGNORMAL << "Deactivate node uuid " << std::hex
              << "0x" << agent->get_uuid().uuid_get_val() << std::dec;

    rs_mtx.lock();
    rs_unregister_mtx(agent);
    node_down_pend.push_back(OM_NodeAgent::agt_cast_ptr(agent));
    rs_mtx.unlock();
}

// om_splice_nodes_pend
// --------------------
//
void
OM_AgentContainer::om_splice_nodes_pend(NodeList *addNodes, NodeList *rmNodes)
{
    rs_mtx.lock();
    addNodes->splice(addNodes->begin(), node_up_pend);
    rmNodes->splice(rmNodes->begin(), node_down_pend);
    rs_mtx.unlock();
}

void
OM_AgentContainer::om_splice_nodes_pend(NodeList *addNodes,
                                        NodeList *rmNodes,
                                        const NodeUuidSet& filter_nodes)
{
    rs_mtx.lock();
    for (NodeUuidSet::const_iterator cit = filter_nodes.cbegin();
         cit != filter_nodes.cend();
         ++cit) {
        NodeList::iterator it = node_up_pend.begin();
        while (it != node_up_pend.end()) {
            if ((*it)->get_uuid() == (*cit)) {
                addNodes->splice(addNodes->begin(), node_up_pend, it);
                break;
            }
            ++it;
        }
        it = node_down_pend.begin();
        while (it != node_down_pend.end()) {
            if ((*it)->get_uuid() == (*cit)) {
                rmNodes->splice(rmNodes->begin(), node_down_pend, it);
                break;
            }
            ++it;
        }
    }
    rs_mtx.unlock();
}

// --------------------------------------------------------------------------------------
// OM DM NodeAgent Container
// --------------------------------------------------------------------------------------
OM_DmContainer::OM_DmContainer() : OM_AgentContainer(fpi::FDSP_DATA_MGR) {}

// -------------------------------------------------------------------------------------
// OM AM NodeAgent Container
// -------------------------------------------------------------------------------------
OM_AmContainer::OM_AmContainer() : OM_AgentContainer(fpi::FDSP_STOR_HVISOR) {}

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
                      new OmContainer(FDSP_ORCH_MGR))
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

    // TODO(Anna) PerfStats class is replaced, not sure yet if we
    // are also pushing stats to OM, so commenting this out for now
    // to remember to port to new stats class yet
    /*
    am_stats = new PerfStats(OrchMgr::om_stor_prefix() + "OM_from_AM",
                             5 * FDS_STAT_DEFAULT_HIST_SLOTS);
    if (am_stats != NULL) {
        am_stats->enable();
    }
    */
}

// om_send_qos_info
// -----------------------
//
static void
om_send_qos_info(fds_uint64_t total_rate, NodeAgent::pointer node)
{
    OM_SmAgent::agt_cast_ptr(node)->om_send_qosinfo(total_rate);
}

// om_update_capacity
// ------------------
//
void
OM_NodeContainer::om_update_capacity(NodeAgent::pointer node,
                                     fds_bool_t b_add)
{
    OM_SmAgent::pointer agent = OM_SmAgent::agt_cast_ptr(node);
    fds_uint64_t old_max_iopc = om_admin_ctrl->getMaxIOPC();

    if (agent->node_get_svc_type() != fpi::FDSP_STOR_HVISOR) {
        if (b_add) {
            om_admin_ctrl->addDiskCapacity(node->node_capability());
        } else {
            om_admin_ctrl->removeDiskCapacity(node->node_capability());
        }

        // if perf capability changed, notify AMs to modify QoS
        // control params accordingly
        fds_uint64_t new_max_iopc = om_admin_ctrl->getMaxIOPC();
        if ((new_max_iopc != 0) &&
            (new_max_iopc != old_max_iopc)) {
            dc_am_nodes->agent_foreach<fds_uint64_t>(new_max_iopc, om_send_qos_info);
        }
    }
}

void
OM_NodeContainer::om_send_me_qosinfo(NodeAgent::pointer me) {
    OM_AmAgent::pointer agent = OM_AmAgent::agt_cast_ptr(me);

    // for now we are just sending total rate to AM
    if (agent->node_get_svc_type() != fpi::FDSP_STOR_HVISOR) return;

    fds_uint64_t max_iopc = om_admin_ctrl->getMaxIOPC();
    if (max_iopc != 0) {
        agent->om_send_qosinfo(max_iopc);
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

// om_activate_service
// -------------------
//
static void
om_activate_services(fds_bool_t activate_sm,
                     fds_bool_t activate_dm,
                     fds_bool_t activate_am,
                     NodeAgent::pointer node)
{
    OM_PmAgent::agt_cast_ptr(node)->send_activate_services(activate_sm,
                                                           activate_dm,
                                                           activate_am);
}

// om_activate_services
//
//
void
OM_NodeContainer::om_cond_bcast_activate_services(fds_bool_t activate_sm,
                                                  fds_bool_t activate_dm,
                                                  fds_bool_t activate_am)
{
    dc_pm_nodes->agent_foreach<fds_bool_t, fds_bool_t, fds_bool_t>
            (activate_sm, activate_dm, activate_am, om_activate_services);
}

// om_activate_service
//
//
Error
OM_NodeContainer::om_activate_node_services(const NodeUuid& node_uuid,
                                            fds_bool_t activate_sm,
                                            fds_bool_t activate_dm,
                                            fds_bool_t activate_am) {
    OM_PmAgent::pointer agent = om_pm_agent(node_uuid);
    if (agent == NULL) {
        LOGERROR << "activate node services: platform service is not "
                 << "running (or node uuid is not correct) on node "
                 << std::hex << node_uuid.uuid_get_val() << std::dec;
        return Error(ERR_NOT_FOUND);
    }

    return agent->send_activate_services(activate_sm,
                                         activate_dm,
                                         activate_am);
}

// om_send_vol_info
// ----------------
//
static void
om_send_vol_info(NodeAgent::pointer me, fds_uint32_t *cnt, VolumeInfo::pointer vol)
{
    /*
     * Only send if not deleted or marked to be deleted.
     */
    if (vol->isDeletePending() || vol->isStateDeleted()) {
        LOGDEBUG << "Dmt not sending Volume to Node :" << vol->vol_get_name()
                 << "; state " << vol->getStateName();
        return;
    }

    (*cnt)++;
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    fpi::FDSP_NotifyVolFlag vol_flag = fpi::FDSP_NOTIFY_VOL_NO_FLAG;
    if (vp->hasCommittedDMT()) {
      vol_flag = fpi::FDSP_NOTIFY_VOL_WILL_SYNC;
    }
    LOGDEBUG << "Dmt Send Volume to Node :" << vol->vol_get_name()
             << "; will sync flag " << vp->hasCommittedDMT();
    OM_NodeAgent::agt_cast_ptr(me)->om_send_vol_cmd(vol,
                                                    fpi::CtrlNotifyVolAddTypeId,
                                                    vol_flag);
}

// om_bcast_vol_list
// -----------------
//
fds_uint32_t
OM_NodeContainer::om_bcast_vol_list(NodeAgent::pointer node)
{
    fds_uint32_t cnt = 0;
    om_volumes->vol_foreach<NodeAgent::pointer, fds_uint32_t *>
                              (node, &cnt, om_send_vol_info);
    LOGDEBUG << "Dmt bcast Volume list :" << cnt;
    return cnt;
}

void
OM_NodeContainer::om_bcast_stream_reg_list(NodeAgent::pointer node) {
    OM_DmAgent::agt_cast_ptr(node)->om_send_stream_reg_cmd(0, true);
}

// om_send_vol_command
// -------------------
// Send the volume command to the node represented by the agent.
//
static Error
om_send_vol_command(fpi::FDSPMsgTypeId cmd_type,
                    fpi::FDSP_NotifyVolFlag vol_flag,
                    VolumeInfo::pointer   vol,
                    NodeAgent::pointer    agent)
{
    return OM_SmAgent::agt_cast_ptr(agent)->om_send_vol_cmd(vol, cmd_type, vol_flag);
}

// om_bcast_vol_create
// -------------------
//
fds_uint32_t
OM_NodeContainer::om_bcast_vol_create(VolumeInfo::pointer vol)
{
    fds_uint32_t errok_sm_count = 0;
    errok_sm_count = dc_sm_nodes->agent_ret_foreach<
        fpi::FDSPMsgTypeId,
        fpi::FDSP_NotifyVolFlag,
        VolumeInfo::pointer>(fpi::CtrlNotifyVolAddTypeId,
                             fpi::FDSP_NOTIFY_VOL_NO_FLAG,
                             vol,
                             om_send_vol_command);

    // for DMs, we should send volume create with WillSync flag set for
    // DMs that are in the middle of addition, and for others no flag
    Error err(ERR_OK);
    RsArray dm_nodes;
    fds_uint32_t dm_count = dc_dm_nodes->rs_container_snapshot(&dm_nodes);
    OM_Module* om = OM_Module::om_singleton();
    ClusterMap* cm = om->om_clusmap_mod();
    NodeUuidSet addedDms = cm->getAddedServices(fpi::FDSP_DATA_MGR);

    fds_uint32_t errok_dm_count = 0;
    for (RsContainer::const_iterator it = dm_nodes.cbegin();
         it != dm_nodes.cend();
         ++it) {
        NodeAgent::pointer cur = agt_cast_ptr<NodeAgent>(*it);
        if ((cur != NULL) &&
            (dc_dm_nodes->rs_get_resource(cur->get_uuid()))) {
            fpi::FDSP_NotifyVolFlag flag = fpi::FDSP_NOTIFY_VOL_NO_FLAG;
            if (addedDms.count(cur->get_uuid()) > 0) {
                flag = fpi::FDSP_NOTIFY_VOL_WILL_SYNC;
            }
            err = om_send_vol_command(fpi::CtrlNotifyVolAddTypeId,
                                      flag, vol, cur);
            if (err.ok()) {
                ++errok_dm_count;
            }
        }
    }

    return (errok_sm_count + errok_dm_count);
}

// om_bcast_vol_modify
// -------------------
//
void
OM_NodeContainer::om_bcast_vol_modify(VolumeInfo::pointer vol)
{
    dc_sm_nodes->agent_ret_foreach<fpi::FDSPMsgTypeId,
                                   fpi::FDSP_NotifyVolFlag,
                                   VolumeInfo::pointer>(fpi::CtrlNotifyVolModTypeId,
                                                    fpi::FDSP_NOTIFY_VOL_NO_FLAG,
                                                    vol, om_send_vol_command);

    dc_dm_nodes->agent_ret_foreach<fpi::FDSPMsgTypeId,
                                   fpi::FDSP_NotifyVolFlag,
                                   VolumeInfo::pointer>(fpi::CtrlNotifyVolModTypeId,
                                                    fpi::FDSP_NOTIFY_VOL_NO_FLAG,
                                                    vol, om_send_vol_command);

    vol->vol_foreach_am<fpi::FDSPMsgTypeId, fpi::FDSP_NotifyVolFlag>
            (fpi::CtrlNotifyVolModTypeId, fpi::FDSP_NOTIFY_VOL_NO_FLAG, om_send_vol_command);
}

// om_bcast_vol_snap
// -------------------
//
fds_uint32_t
OM_NodeContainer::om_bcast_vol_snap(VolumeInfo::pointer vol)
{
    fds_uint32_t errok_dm_nodes = 0;
    errok_dm_nodes = dc_dm_nodes->agent_ret_foreach<
        fpi::FDSPMsgTypeId,
        fpi::FDSP_NotifyVolFlag,
        VolumeInfo::pointer>(fpi::CtrlNotifySnapVolTypeId,
                             fpi::FDSP_NOTIFY_VOL_NO_FLAG,
                             vol, om_send_vol_command);
    return errok_dm_nodes;
}

// om_bcast_vol_detach
// -------------------
//
fds_uint32_t
OM_NodeContainer::om_bcast_vol_detach(VolumeInfo::pointer vol)
{
    return vol->vol_foreach_am<fpi::FDSPMsgTypeId,
                               fpi::FDSP_NotifyVolFlag>(fpi::CtrlNotifyVolRemoveTypeId,
                                                        fpi::FDSP_NOTIFY_VOL_NO_FLAG,
                                                        om_send_vol_command);
}


// om_bcast_vol_delete
// -------------------
//
fds_uint32_t
OM_NodeContainer::om_bcast_vol_delete(VolumeInfo::pointer vol, fds_bool_t check_only)
{
    fds_uint32_t count = 0;
    fpi::FDSP_NotifyVolFlag vol_flag = fpi::FDSP_NOTIFY_VOL_NO_FLAG;
    if (check_only) {
        vol_flag = fpi::FDSP_NOTIFY_VOL_CHECK_ONLY;
    }

    if (!check_only) {
        dc_sm_nodes->agent_ret_foreach<fpi::FDSPMsgTypeId,
                                       fpi::FDSP_NotifyVolFlag,
                                       VolumeInfo::pointer>(fpi::CtrlNotifyVolRemoveTypeId,
                                                            vol_flag, vol,
                                                            om_send_vol_command);
        count += dc_sm_nodes->rs_available_elm();
    }

    dc_dm_nodes->agent_ret_foreach<fpi::FDSPMsgTypeId,
                                   fpi::FDSP_NotifyVolFlag,
                                   VolumeInfo::pointer>(fpi::CtrlNotifyVolRemoveTypeId,
                                                        vol_flag, vol,
                                                        om_send_vol_command);
    count += dc_dm_nodes->rs_available_elm();

    return count;
}

// om_send_node_command
// --------------------
// Plugin to send a generic node command to all nodes.  Plugin to node iterator.
//
static void
om_send_node_throttle_lvl(fpi::FDSP_ThrottleMsgTypePtr msg,
                        fds_uint32_t *cnt, NodeAgent::pointer node)
{
    (*cnt)++;
    OM_SmAgent::agt_cast_ptr(node)->om_send_node_throttle_lvl(msg);
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
    fds_uint32_t count = 0;
    fpi::FDSP_ThrottleMsgTypePtr throttle(new fpi::FDSP_ThrottleMsgType);

    throttle->domain_id      = DEFAULT_LOC_DOMAIN_ID;
    throttle->throttle_level = throttle_level;

    dc_am_nodes->agent_foreach<fpi::FDSP_ThrottleMsgTypePtr, \
            fds_uint32_t *>(throttle, &count, om_send_node_throttle_lvl);
}

// om_set_throttle_lvl
// -------------------
//
void
OM_NodeContainer::om_set_throttle_lvl(float level)
{
    om_cur_throttle_level = level;

    LOGNOTIFY << "Setting throttle level for local domain at " << level << std::endl;

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

// om_send_dlt
// -----------------------
//
static Error
om_send_dmt(const DMTPtr& curDmt, NodeAgent::pointer agent)
{
    return OM_NodeAgent::agt_cast_ptr(agent)->om_send_dmt(curDmt);
}

static Error
om_send_dmt_x(const DMTPtr& curDmt, NodeAgent::pointer agent)
{
    return OM_NodeAgent::agt_cast_ptr(agent)->om_send_dmt_x(curDmt);
}

// om_bcast_dmt
// -------------
//
fds_uint32_t
OM_NodeContainer::om_bcast_dmt(fpi::FDSP_MgrIdType svc_type,
                               const DMTPtr& curDmt)
{
    fds_uint32_t count = 0;
    if (svc_type == fpi::FDSP_DATA_MGR) {
        count += dc_dm_nodes->agent_ret_foreach<const DMTPtr&>(curDmt, om_send_dmt);
        LOGDEBUG << "Sent DMT to " << count << " DM services successfully";
    } else if (svc_type == fpi::FDSP_STOR_MGR) {
        count += dc_sm_nodes->agent_ret_foreach<const DMTPtr&>(curDmt, om_send_dmt);
        LOGDEBUG << "Sent DMT to " << count << " SM services successfully";
    } else {
        // this method must only be called for either DM, SM or AM!
        fds_verify(svc_type == fpi::FDSP_STOR_HVISOR);
        count += dc_am_nodes->agent_ret_foreach<const DMTPtr&>(curDmt, om_send_dmt);
        LOGDEBUG << "Sent DMT to " << count << " AM services successfully";
    }
#ifdef LLIU_WORK_IN_PROGRESS
    //   the following is to test for PM to receive dmt
    LOGNORMAL << " try to send out dmt to pm to shared memory see see ";
    dc_pm_nodes->agent_ret_foreach<const DMTPtr&>(curDmt, om_send_dmt_x);
#endif
    return count;
}

// om_send_dmt_close
// -----------------------
//
static Error
om_send_dmt_close(fds_uint64_t dmt_version, NodeAgent::pointer agent)
{
    return OM_DmAgent::agt_cast_ptr(agent)->om_send_dmt_close(dmt_version);
}

// om_bcast_dmt_close
// ------------------
// Broadcasts to DMs that are already in cluster map! Just added DMs that
// are not included in DMT must not get DMT close message (which may arrive
// without DMT update if DM was added right in the middle)
// @return number of nodes we sent the message to (and
// we are waiting for that many responses)
//
fds_uint32_t
OM_NodeContainer::om_bcast_dmt_close(fds_uint64_t dmt_version)
{
    Error err(ERR_OK);
    OM_Module* om = OM_Module::om_singleton();
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    ClusterMap* cm = om->om_clusmap_mod();
    fds_uint32_t count = 0;

    // iterate over DMs that are in cluster map, and send dmt close
    // mesage to each of them
    for (ClusterMap::const_dm_iterator cit = cm->cbegin_dm();
         cit != cm->cend_dm();
         ++cit) {
        OM_DmAgent::pointer dm_agent = loc_domain->om_dm_agent(cit->first);
        err = dm_agent->om_send_dmt_close(dmt_version);
        if (err.ok()) ++count;
    }
    LOGDEBUG << "Send DMT close to " << count << " DMs successfully";
    return count;
}


// om_send_dlt
// -----------------------
//
static Error
om_send_dlt(const DLT* curDlt, NodeAgent::pointer agent)
{
    return OM_SmAgent::agt_cast_ptr(agent)->om_send_dlt(curDlt);
}

// om_bcast_dlt
// ------------
//
fds_uint32_t
OM_NodeContainer::om_bcast_dlt(const DLT* curDlt, fds_bool_t sm_only)
{
    fds_uint32_t count = 0;
    count = dc_sm_nodes->agent_ret_foreach<const DLT*>(curDlt, om_send_dlt);

#ifdef LLIU_WORK_IN_PROGRESS
    //   the following is to test for PM to receive dlt
    LOGNORMAL << " try to send out dlt to pm to shared memory see see ";
    dc_pm_nodes->agent_ret_foreach<const DLT*>(curDlt, om_send_dlt);
#endif

    if (sm_only) {
        return count;
    }

    count += dc_dm_nodes->agent_ret_foreach<const DLT*>(curDlt, om_send_dlt);
    count += dc_am_nodes->agent_ret_foreach<const DLT*>(curDlt, om_send_dlt);

    LOGDEBUG << "Sent dlt to " << count << " nodes successfully";
    return count;
}

// om_send_dlt_close
// -----------------------
//
static Error
om_send_dlt_close(fds_uint64_t cur_dlt_version, NodeAgent::pointer agent)
{
    return OM_SmAgent::agt_cast_ptr(agent)->om_send_dlt_close(cur_dlt_version);
}

// om_bcast_dlt_close
// ------------------
// @return number of nodes we sent the message to (and
// we are waiting for that many responses)
//
fds_uint32_t
OM_NodeContainer::om_bcast_dlt_close(fds_uint64_t cur_dlt_version)
{
    fds_uint32_t count = 0;
    count = dc_sm_nodes->agent_ret_foreach<fds_uint64_t>(cur_dlt_version,
                                                         om_send_dlt_close);
    LOGDEBUG << "Send dlt close to " << count << " nodes successfully";
    return count;
}

// om_send_dlt_close
// -----------------------
//
static void
om_send_scavenger_cmd(FDS_ProtocolInterface::FDSP_ScavengerCmd cmd,
                      NodeAgent::pointer agent) {
    OM_SmAgent::agt_cast_ptr(agent)->om_send_scavenger_cmd(cmd);
}

//
// For now sends scavenger start message
// TODO(xxx) extend to other scavenger commands
//
void
OM_NodeContainer::om_bcast_scavenger_cmd(FDS_ProtocolInterface::FDSP_ScavengerCmd cmd)
{
    dc_sm_nodes->agent_foreach<FDS_ProtocolInterface::FDSP_ScavengerCmd>(
        cmd, om_send_scavenger_cmd);
}


static void
om_send_stream_reg_cmd(fds_int32_t regId, fds_bool_t bAll,
                       NodeAgent::pointer agent) {
    OM_DmAgent::agt_cast_ptr(agent)->om_send_stream_reg_cmd(regId, bAll);
}

void
OM_NodeContainer::om_bcast_stream_register_cmd(fds_int32_t regId,
                                               fds_bool_t bAll)
{
    dc_dm_nodes->agent_foreach<fds_int32_t, fds_bool_t>(regId, bAll, om_send_stream_reg_cmd);
}


}  // namespace fds

/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <vector>
#include <string>
#include <orchMgr.h>
#include <NetSession.h>
#include <OmResources.h>
#include <net/SvcRequest.h>
#include <fiu-local.h>
#include <fdsp/svc_types_types.h>
#include <net/PlatNetSvcHandler.h>
#include <net/PlatNetSvcHandler.h>
#include "platform/node_data.h"
#include "platform/platform.h"
#include <net/net_utils.h>
#include <fds_module_provider.h>
#include <net/SvcMgr.h>
#undef LOGGERPTR
#define LOGGERPTR orchMgr->GetLog()
namespace fds {

FDSP_ConfigPathReqHandler::FDSP_ConfigPathReqHandler(OrchMgr *oMgr) {
    orchMgr = oMgr;
}

int32_t FDSP_ConfigPathReqHandler::ActivateNode(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_ActivateOneNodeType& act_node_msg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t FDSP_ConfigPathReqHandler::ActivateNode(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_ActivateOneNodeTypePtr& act_node_msg) {

    static bool activate_node_services_done = false;
    bool activate_dm = act_node_msg->activate_dm;
    if (activate_node_services_done) {
        if (act_node_msg->activate_dm) {
             LOGWARN << "Trying to activate DM after first issue of activate_services";
        }
        activate_dm = false;
    }
    activate_node_services_done = true;

    Error err(ERR_OK);
    try {
        int domain_id = act_node_msg->domain_id;
        // use default domain for now
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        NodeUuid node_uuid((act_node_msg->node_uuid).uuid);

        LOGNORMAL << "Received Activate Node Req for domain " << domain_id
                  << " node uuid " << std::hex
                  << node_uuid.uuid_get_val() << std::dec;

        err = local->om_activate_node_services(node_uuid,
                                               act_node_msg->activate_sm,
                                               activate_dm,
                                               act_node_msg->activate_am);
    }
    catch(...) {
        LOGERROR << "Orch Mgr encountered exception while "
                 << "processing activate all nodes";
        err = Error(ERR_NOT_FOUND);  // need some better error code
    }

    return err.GetErrno();
}

int32_t FDSP_ConfigPathReqHandler::ScavengerCommand(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_ScavengerType& gc_msg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t FDSP_ConfigPathReqHandler::ScavengerCommand(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_ScavengerTypePtr& gc_msg) {
    switch (gc_msg->cmd) {
        case FDS_ProtocolInterface::FDSP_SCAVENGER_ENABLE:
            LOGNOTIFY << "Received scavenger enable command";
            break;
        case FDS_ProtocolInterface::FDSP_SCAVENGER_DISABLE:
            LOGNOTIFY << "Received scavenger disable command";
            break;
        case FDS_ProtocolInterface::FDSP_SCAVENGER_START:
            LOGNOTIFY << "Received scavenger start command";
            break;
        case FDS_ProtocolInterface::FDSP_SCAVENGER_STOP:
            LOGNOTIFY << "Received scavenger stop command";
            break;
    };

    // send scavenger start message to all SMs
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_bcast_scavenger_cmd(gc_msg->cmd);
    return 0;
}

void FDSP_ConfigPathReqHandler::ListServices(
    std::vector<FDSP_Node_Info_Type> & ret,
    const FDSP_MsgHdrType& fdsp_msg) {
    // Do nothing
}

void add_to_vector(std::vector<fpi::FDSP_Node_Info_Type> &vec,  // NOLINT
                          NodeAgent::pointer ptr) {

    fpi::FDSP_RegisterNodeType nodeData;

    fpi::SvcInfo svcInfo;
    fpi::SvcUuid svcUuid;
    svcUuid.svc_uuid = ptr->rs_get_uuid().uuid_get_val();
    /* Getting from svc map.  Should be able to get it from config db as well */
    if (!MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, svcInfo)) {
        GLOGWARN << "could not fing svcinfo for uuid:" << svcUuid.svc_uuid;
        return;
    }

    fpi::FDSP_Node_Info_Type nodeInfo = fpi::FDSP_Node_Info_Type();
    nodeInfo.node_uuid = SvcMgr::mapToSvcUuid(svcUuid, fpi::FDSP_PLATFORM).svc_uuid;
    nodeInfo.service_uuid = svcUuid.svc_uuid;
    nodeInfo.node_name = svcInfo.name;
    nodeInfo.node_type = svcInfo.svc_type;
    nodeInfo.node_state = ptr->node_state();
    nodeInfo.ip_lo_addr =  net::ipString2Addr(svcInfo.ip);
    nodeInfo.control_port = nodeData.control_port;
    nodeInfo.data_port = svcInfo.svc_port;
    nodeInfo.migration_port = nodeData.migration_port;
    vec.push_back(nodeInfo);
}

void FDSP_ConfigPathReqHandler::ListServices(
    std::vector<FDSP_Node_Info_Type> &vec,
    boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg) {

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();

    local->om_sm_nodes()->
        agent_foreach<std::vector<FDSP_Node_Info_Type> &>(vec, add_to_vector);
    local->om_am_nodes()->
        agent_foreach<std::vector<FDSP_Node_Info_Type> &>(vec, add_to_vector);
    local->om_dm_nodes()->
        agent_foreach<std::vector<FDSP_Node_Info_Type> &>(vec, add_to_vector);
    local->om_pm_nodes()->
        agent_foreach<std::vector<FDSP_Node_Info_Type> &>(vec, add_to_vector);
}

void FDSP_ConfigPathReqHandler::ListVolumes(
    std::vector<FDS_ProtocolInterface::FDSP_VolumeDescType> & _return,
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

static void
add_vol_to_vector(std::vector<FDS_ProtocolInterface::FDSP_VolumeDescType> &vec,  // NOLINT
                  VolumeInfo::pointer vol) {
    FDS_ProtocolInterface::FDSP_VolumeDescType voldesc;
    vol->vol_fmt_desc_pkt(&voldesc);
    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "Volume in list: " << voldesc.vol_name << ":"
            << std::hex << voldesc.volUUID << std::dec
            << "min iops " << voldesc.iops_min << ",max iops "
            << voldesc.iops_max << ", prio " << voldesc.rel_prio;
    vec.push_back(voldesc);
}

void FDSP_ConfigPathReqHandler::ListVolumes(
    std::vector<FDS_ProtocolInterface::FDSP_VolumeDescType> & _return,
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg) {
    LOGNOTIFY<< "OM received ListVolumes message";
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer vols = local->om_vol_mgr();
    // list volumes that are not in 'delete pending' state
    vols->vol_up_foreach<std::vector<FDSP_VolumeDescType> &>(_return, add_vol_to_vector);
}

FDSP_OMControlPathReqHandler::FDSP_OMControlPathReqHandler(
    OrchMgr *oMgr)
    : PlatNetSvcHandler(oMgr) 
    {
    orchMgr = oMgr;

    // REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyMigrationStatus, migrationDone);
}

void FDSP_OMControlPathReqHandler::RegisterNode(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_RegisterNodeType& reg_node_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void FDSP_OMControlPathReqHandler::RegisterNode(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr& reg_node_req) {
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    NodeUuid new_node_uuid;

    if (reg_node_req->node_uuid.uuid == 0) {
        LOGERROR << "Refuse to register a node without valid uuid: node_type "
            << reg_node_req->node_type << ", name " << reg_node_req->node_name;
        return;
    }
    new_node_uuid.uuid_set_type(reg_node_req->node_uuid.uuid, reg_node_req->node_type);
    Error err = domain->om_reg_node_info(new_node_uuid, reg_node_req);
    if (!err.ok()) {
        LOGERROR << "Node Registration failed for "
                 << reg_node_req->node_name << ":" << std::hex
                 << new_node_uuid.uuid_get_val() << std::dec
                 << " node_type " << reg_node_req->node_type
                 << ", result: " << err.GetErrstr();
        return;
    }
    LOGNORMAL << "Done Registered new node " << reg_node_req->node_name << std::hex
              << ", node uuid " << reg_node_req->node_uuid.uuid
              << ", svc uuid " << new_node_uuid.uuid_get_val()
              << ", node type " << reg_node_req->node_type << std::dec;
}

void FDSP_OMControlPathReqHandler::migrationDone(boost::shared_ptr<fpi::AsyncHdr>& hdr,
        boost::shared_ptr<fpi::CtrlNotifyMigrationStatus>& status) {
}

}  // namespace fds

/*
 * Copyright 2013 by Formations Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <iostream>  // NOLINT
#include <thrift/protocol/TDebugProtocol.h>
#include <net/RpcFunc.h>
#include <om-svc-handler.h>
#include <OmResources.h>
#include <OmDeploy.h>
#include <OmDmtDeploy.h>
#include <OmDataPlacement.h>
#include <OmVolumePlacement.h>
#include <orch-mgr/om-service.h>
#include <platform/net-plat-shared.h>
#include <util/Log.h>

namespace fds {

OmSvcHandler::~OmSvcHandler() {}

OmSvcHandler::OmSvcHandler()
{
    om_mod = OM_NodeDomainMod::om_local_domain();
    // REGISTER_FDSP_MSG_HANDLER(fpi::NodeInfoMsg, om_node_info);
    // REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, om_node_svc_info);


    /* svc->om response message */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolAdd, NotifyAddVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolRemove, NotifyRmVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolMod, NotifyModVol);
}

// om_svc_state_chg
// ----------------
//
void
OmSvcHandler::om_node_svc_info(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                               boost::shared_ptr<fpi::NodeSvcInfo> &msg)
{
    LOGNORMAL << "Service up";
}

// om_node_info
// ------------
//
void
OmSvcHandler::om_node_info(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                           boost::shared_ptr<fpi::NodeInfoMsg> &node)
{
    LOGNORMAL << "Node up";
}

void
OmSvcHandler::NotifyAddVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                           boost::shared_ptr<fpi::CtrlNotifyVolAdd> &vol_msg)
{
    LOGNOTIFY << "OM received response for NotifyAddVol from node " << std::hex
              << hdr->msg_src_uuid.svc_uuid << " for volume "
              << "[" << vol_msg->vol_desc.vol_name << ":"
              << std::hex << vol_msg->vol_desc.volUUID << std::dec
              << "] Result: " << hdr->msg_code;

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    volumes->om_notify_vol_resp(om_notify_vol_add,
                                hdr->msg_src_uuid.svc_uuid, hdr->msg_code,
                                vol_msg->vol_info.vol_name,
                                vol_msg->vol_desc.volUUID);
}

void
OmSvcHandler::NotifyRmVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                           boost::shared_ptr<fpi::CtrlNotifyVolRemove> &vol_msg)
{
    fds_bool_t check_only = (vol_msg->vol_flag == fpi::FDSP_NOTIFY_VOL_CHECK_ONLY);
    LOGNOTIFY << "OM received response for NotifyRmVol (check only "
              << check_only << ") from node " << std::hex
              << hdr->msg_src_uuid.svc_uuid << " for volume "
              << "[" << vol_msg->vol_desc.vol_name << ":"
              << std::hex << vol_msg->vol_desc.volUUID << std::dec
              << "] Result: " << hdr->msg_code;

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    om_vol_notify_t type = check_only ? om_notify_vol_rm_chk : om_notify_vol_rm;
    volumes->om_notify_vol_resp(type,
                                hdr->msg_src_uuid.svc_uuid, hdr->msg_code,
                                vol_msg->vol_desc.vol_name,
                                vol_msg->vol_desc.volUUID);
}
void
OmSvcHandler::NotifyModVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlNotifyVolMod> &vol_msg)
{
    LOGNOTIFY << "OM received response for NotifyModVol from node "
              << hdr->msg_src_uuid.svc_uuid << " for volume "
              << "[" << vol_msg->vol_desc.vol_name << ":"
              << std::hex << vol_msg->vol_desc.volUUID << std::dec
              << "] Result: " << hdr->msg_code;

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    volumes->om_notify_vol_resp(om_notify_vol_mod,
                                hdr->msg_src_uuid.svc_uuid, hdr->msg_code,
                                vol_msg->vol_desc.vol_name,
                                vol_msg->vol_desc.volUUID);
}

}  //  namespace fds

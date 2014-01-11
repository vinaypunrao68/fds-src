/*                                                                           
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>

namespace fds {

OrchMgr::FDSP_ControlPathRespHandler::FDSP_ControlPathRespHandler(OrchMgr *oMgr) {
    orchMgr = oMgr;
}
void OrchMgr::FDSP_ControlPathRespHandler::NotifyAddVolResp(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_NotifyVolType& not_add_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyAddVolResp(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_add_vol_resp) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::notification)
            << "OrchMgr: received response for NotifyAddVol";
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyRmVolResp(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_NotifyVolType& not_rm_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyRmVolResp(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_rm_vol_resp) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::notification)
            << "OrchMgr: received response for NotifyRmVol";
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyModVolResp(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_NotifyVolType& not_mod_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyModVolResp(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_mod_vol_resp) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::notification)
            << "OrchMgr: received response for NotifyModVol";
}

void OrchMgr::FDSP_ControlPathRespHandler::AttachVolResp(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_AttachVolType& atc_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_ControlPathRespHandler::AttachVolResp(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_AttachVolTypePtr& atc_vol_resp) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::notification)
            << "OrchMgr: received response for AttachVol";
}

void OrchMgr::FDSP_ControlPathRespHandler::DetachVolResp(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_AttachVolType& dtc_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_ControlPathRespHandler::DetachVolResp(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_AttachVolTypePtr& dtc_vol_resp) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::notification)
            << "OrchMgr: received response for DetachVol";
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyNodeAddResp(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyNodeAddResp(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::notification)
            << "OrchMgr: received response for NotifyNodeAdd";
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyNodeRmvResp(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyNodeRmvResp(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::notification)
            << "OrchMgr: received response for NotifyNodeRmv";
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyDLTUpdateResp(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_DLT_Type& dlt_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyDLTUpdateResp(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_DLT_TypePtr& dlt_info_resp) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::notification)
            << "OrchMgr: received response for NotifyDLTUpdate";
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyDMTUpdateResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_DMT_Type& dmt_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_ControlPathRespHandler::NotifyDMTUpdateResp(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_DMT_TypePtr& dmt_info_resp) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::notification)
            << "OrchMgr: received response for NotifyDMTUpdate";
}

}  // namespace fds

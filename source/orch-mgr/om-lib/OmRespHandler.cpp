/*                                                                           
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>

namespace fds {

FDSP_ControlPathRespHandler::FDSP_ControlPathRespHandler(OrchMgr *oMgr) {
    orchMgr = oMgr;
}

void FDSP_ControlPathRespHandler::NotifyDMTUpdateResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_DMT_Resp_Type& dmt_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void FDSP_ControlPathRespHandler::NotifyDMTUpdateResp(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_DMT_Resp_TypePtr& dmt_info_resp) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::notification)
            << "OrchMgr: received response for NotifyDMTUpdate";
}

void FDSP_ControlPathRespHandler::NotifyDMTCloseResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_DMT_Resp_Type& dmt_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
FDSP_ControlPathRespHandler::NotifyDMTCloseResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_DMT_Resp_TypePtr& dmt_resp) {
}

void
FDSP_ControlPathRespHandler::PushMetaDMTResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_PushMeta& push_meta_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}
void
FDSP_ControlPathRespHandler::PushMetaDMTResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_PushMetaPtr& push_meta_resp) {
}

}  // namespace fds

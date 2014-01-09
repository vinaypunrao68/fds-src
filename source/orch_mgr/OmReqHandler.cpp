/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>

namespace fds {

OrchMgr::FDSP_ConfigPathReqHandler::ReqCfgHandler(OrchMgr *oMgr) {
    orchMgr = oMgr;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::CreateVol(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_CreateVolType& crt_vol_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::CreateVol(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_CreateVolTypePtr& crt_vol_req) {

    int err = 0;
    try {
        err = orchMgr->CreateVol(fdsp_msg, crt_vol_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing create volume";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::DeleteVol(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_DeleteVolType& del_vol_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::DeleteVol(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_DeleteVolTypePtr& del_vol_req) {

    int err = 0;
    try {
        err = orchMgr->DeleteVol(fdsp_msg, del_vol_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing delete volume";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::ModifyVol(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_ModifyVolType& mod_vol_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::ModifyVol(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_ModifyVolTypePtr& mod_vol_req) {

    int err = 0;
    try {
        err = orchMgr->ModifyVol(fdsp_msg, mod_vol_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing modify volume";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::CreatePolicy(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_CreatePolicyType& crt_pol_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::CreatePolicy(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr& crt_pol_req) {

    int err = 0;
    try {
        err = orchMgr->CreatePolicy(fdsp_msg, crt_pol_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing create policy";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::DeletePolicy(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_DeletePolicyType& del_pol_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::DeletePolicy(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr& del_pol_req) {

    int err = 0;
    try {
        err = orchMgr->DeletePolicy(fdsp_msg, del_pol_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing delete policy";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::ModifyPolicy(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_ModifyPolicyType& mod_pol_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::ModifyPolicy(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr& mod_pol_req) {

    int err = 0;
    try {
        err = orchMgr->ModifyPolicy(fdsp_msg, mod_pol_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing modify policy";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::AttachVol(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_AttachVolCmdType& atc_vol_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::AttachVol(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr& atc_vol_req) {

    int err = 0;
    try {
        err = orchMgr->AttachVol(fdsp_msg, atc_vol_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing attach volume";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::DetachVol(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_AttachVolCmdType& dtc_vol_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::DetachVol(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr& dtc_vol_req) {

    int err = 0;
    try {
        err = orchMgr->DetachVol(fdsp_msg, dtc_vol_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing detach volume";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::AssociateRespCallback(
    const int64_t ident) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::AssociateRespCallback(
    boost::shared_ptr<int64_t>& ident) {
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::CreateDomain(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_CreateDomainType& crt_dom_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::CreateDomain(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_CreateDomainTypePtr& crt_dom_req) {

    int err = 0;
    try {
        err = orchMgr->CreateDomain(fdsp_msg, crt_dom_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing create domain";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::DeleteDomain(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_CreateDomainType& del_dom_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::DeleteDomain(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_CreateDomainTypePtr& del_dom_req) {

    int err = 0;
    try {
        err = orchMgr->CreateDomain(fdsp_msg, crt_dom_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing create domain";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::SetThrottleLevel(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_ThrottleMsgType& throttle_msg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::SetThrottleLevel(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_ThrottleMsgTypePtr& throttle_msg) {

    int err = 0;
    try {
        err = orchMgr->SetThrottleLevel(fdsp_msg, throttle_msg);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing set throttle level";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::GetVolInfo(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_GetVolInfoReqType& vol_info_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::GetVolInfo(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_GetVolInfoReqTypePtr& vol_info_req) {

    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::GetDomainStats(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_GetDomainStatsType& get_stats_msg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::GetDomainStats(
    ::FDS_ProtocolInterface::FDSP_MsgHdrType_Ptr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_GetDomainStatsType_Ptr& get_stats_msg) {

    int err = 0;
    try {
        err = orchMgr->GetDomainStats(fdsp_msg, get_stats_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing get domain stats";
        return -1;
    }

    return err;
}


OrchMgr::FDSP_OMControlPathReqHandler::FDSP_OMControlPathReqHandler(
    OrchMgr *oMgr) {
    orchMgr = oMgr;
}

void OrchMgr::FDSP_OMControlPathReqHandler::CreateBucket(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_CreateVolType& crt_buck_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::CreateBucket(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_CreateVolTypePtr& crt_buck_req) {

    try {
        orchMgr->CreateVol(fdsp_msg, crt_buck_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds::fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing create bucket";
    }
}

void OrchMgr::FDSP_OMControlPathReqHandler::DeleteBucket(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_DeleteVolType& del_buck_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::DeleteBucket(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_DeleteVolTypePtr& del_buck_req) {
    orchMgr->DeleteVol(fdsp_msg, del_buck_req);
}

void OrchMgr::FDSP_OMControlPathReqHandler::ModifyBucket(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_ModifyVolType& mod_buck_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::ModifyBucket(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_ModifyVolTypePtr& mod_buck_req) {
    orchMgr->ModifyVol(fdsp_msg, mod_buck_req);
}

void OrchMgr::FDSP_OMControlPathReqHandler::AttachBucket(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_AttachVolCmdType& atc_buck_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::AttachBucket(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr& atc_buck_req) {
    orchMgr->AttachVol(fdsp_msg, atc_buck_req);
}

void OrchMgr::FDSP_OMControlPathReqHandler::RegisterNode(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_RegisterNodeType& reg_node_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::RegisterNode(
    ::FDS_ProtocolInterface::FDSP_MsgHdrType_Ptr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_RegisterNodeType_Ptr& reg_node_req) {
    orchMgr->RegisterNode(fdsp_msg, reg_node_req);
}

void OrchMgr::FDSP_OMControlPathReqHandler::NotifyQueueFull(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_NotifyQueueStateType& queue_state_info){
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::NotifyQueueFull(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_NotifyQueueStateTypePtr& queue_state_info) {
    orchMgr->NotifyQueueFull(fdsp_msg, queue_state_info);
}

void OrchMgr::FDSP_OMControlPathReqHandler::NotifyPerfstats(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_PerfstatsType& push_stats_msg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::NotifyPerfstats(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_PerfstatsTypePtr& push_stats_msg) {

    orchMgr->NotifyPerfstats(fdsp_msg, perf_stats_msg);
}

void OrchMgr::FDSP_OMControlPathReqHandler::TestBucket(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_TestBucket& test_buck_msg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::TestBucket(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_TestBucketPtr& test_buck_msg) {
    orchMgr->TestBucket(fdsp_msg, test_buck_msg);
}

void OrchMgr::FDSP_OMControlPathReqHandler::GetDomainStats(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_GetDomainStatsType& get_stats_msg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::GetDomainStats(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_GetDomainStatsTypePtr& get_stats_msg) {

    try {
        orchMgr->GetDomainStats(fdsp_msg, get_stats_msg);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing get domain stats";
    }
}

}  // namespace fds


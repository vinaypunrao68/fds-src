/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>
#include <OmResources.h>

namespace fds {

OrchMgr::FDSP_ConfigPathReqHandler::FDSP_ConfigPathReqHandler(OrchMgr *oMgr) {
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
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        err = local->om_create_vol(crt_vol_req);
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
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        err = local->om_delete_vol(del_vol_req);
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
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        err = local->om_modify_vol(mod_vol_req);
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
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        err = local->om_attach_vol(fdsp_msg, atc_vol_req);
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
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        err = local->om_detach_vol(fdsp_msg, dtc_vol_req);
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
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        err = domain->om_create_domain(crt_dom_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing create domain";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::RemoveNode(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_RemoveNodeType& rm_node_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::RemoveNode(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_RemoveNodeTypePtr& rm_node_req) {

    int err = 0;
    try {
        err = orchMgr->RemoveNode(fdsp_msg, rm_node_req);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing rmv node";
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
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        err = domain->om_delete_domain(del_dom_req);
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
        assert((throttle_msg->throttle_level >= -10) &&
               (throttle_msg->throttle_level <= 10));
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        local->om_set_throttle_lvl(throttle_msg->throttle_level);
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
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_GetDomainStatsTypePtr& get_stats_msg) {

    int err = 0;
    try {
        int domain_id = get_stats_msg->domain_id;
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();

        FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "Received GetDomainStats Req for domain " << domain_id;

        /* Use default domain for now... */
        local->om_send_bucket_stats(5, fdsp_msg->src_node_name, fdsp_msg->req_cookie);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing get domain stats";
        return -1;
    }

    return err;
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
        int domain_id = get_stats_msg->domain_id;
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();

        FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "Received GetDomainStats Req for domain " << domain_id;

        /* Use default domain for now... */
        local->om_send_bucket_stats(5, fdsp_msg->src_node_name, fdsp_msg->req_cookie);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing get domain stats";
    }
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::applyTierPolicy(
    const ::FDS_ProtocolInterface::tier_pol_time_unit& policy) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::applyTierPolicy(
    ::FDS_ProtocolInterface::tier_pol_time_unitPtr& policy) {
    int err = 0;
    try {
        err = orchMgr->ApplyTierPolicy(policy);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing apply tier policy";
        return -1;
    }

    return err;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::auditTierPolicy(
    const ::FDS_ProtocolInterface::tier_pol_audit& audit) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return 0;
}

int32_t OrchMgr::FDSP_ConfigPathReqHandler::auditTierPolicy(
    ::FDS_ProtocolInterface::tier_pol_auditPtr& audit) {
    int err = 0;
    try {
        err = orchMgr->AuditTierPolicy(audit);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing audit tier policy";
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
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        local->om_create_vol(crt_buck_req);
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
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_delete_vol(del_buck_req);
}

void OrchMgr::FDSP_OMControlPathReqHandler::ModifyBucket(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_ModifyVolType& mod_buck_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::ModifyBucket(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_ModifyVolTypePtr& mod_buck_req) {
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_modify_vol(mod_buck_req);
}

void OrchMgr::FDSP_OMControlPathReqHandler::AttachBucket(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_AttachVolCmdType& atc_buck_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::AttachBucket(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr& atc_buck_req) {
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_attach_vol(fdsp_msg, atc_buck_req);
}

void OrchMgr::FDSP_OMControlPathReqHandler::RegisterNode(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_RegisterNodeType& reg_node_req) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::RegisterNode(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr& reg_node_req) {
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
    const ::FDS_ProtocolInterface::FDSP_PerfstatsType& perf_stats_msg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::NotifyPerfstats(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_PerfstatsTypePtr& perf_stats_msg) {

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

void OrchMgr::FDSP_OMControlPathReqHandler::NotifyMigrationDone(
    const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const ::FDS_ProtocolInterface::FDSP_MigrationStatusType& status_msg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void OrchMgr::FDSP_OMControlPathReqHandler::NotifyMigrationDone(
    ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    ::FDS_ProtocolInterface::FDSP_MigrationStatusTypePtr& status_msg) {

    try {
        FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "Received migration done notification from node "
            << fdsp_msg->src_node_name;

        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();

        // TODO(Anna) Should we use node names or node uuids directly in
        // fdsp messages? for now getting uuid from hashing the name
        NodeUuid node_uuid(fds_get_uuid64(fdsp_msg->src_node_name));
        Error err = domain->om_recv_migration_done(node_uuid, status_msg->DLT_version);
    }
    catch(...) {
        FDS_PLOG_SEV(orchMgr->GetLog(), fds_log::error)
                << "Orch Mgr encountered exception while "
                << "processing migration done request";
    }
}

}  // namespace fds


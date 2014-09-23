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
#include <orchMgr.h>

namespace fds {

OmSvcHandler::~OmSvcHandler() {}

OmSvcHandler::OmSvcHandler()
{
    om_mod = OM_NodeDomainMod::om_local_domain();
    // REGISTER_FDSP_MSG_HANDLER(fpi::NodeInfoMsg, om_node_info);
    // REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, om_node_svc_info);


    /* svc->om response message */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlTestBucket, TestBucket);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlGetBucketStats, GetBucketStats);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlCreateBucket, CreateBucket);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlDeleteBucket, DeleteBucket);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlModifyBucket, ModifyBucket);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlPerfStats, PerfStats);
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
OmSvcHandler::TestBucket(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlTestBucket> &msg)
{
    LOGNORMAL << " receive test bucket msg";
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_test_bucket(hdr, &msg->tbmsg);
}

void
OmSvcHandler::    GetBucketStats(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlGetBucketStats> &msg)
{
    fpi::FDSP_GetDomainStatsType * get_stats_msg = &msg->gds;

    try {
        int domain_id = get_stats_msg->domain_id;
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        NodeUuid svc_uuid(hdr->msg_src_uuid.svc_uuid);

        LOGNORMAL << "Received GetDomainStats Req for domain " << domain_id
                  << " from node " << hdr->msg_src_uuid.svc_uuid << ":"
                  << std::hex << svc_uuid.uuid_get_val() << std::dec;

        /* Use default domain for now... */
        local->om_send_bucket_stats(5, svc_uuid, msg->req_cookie);
    }
    catch(...) {
        LOGERROR << "Orch Mgr encountered exception while "
                 << "processing get domain stats";
    }
}

void
OmSvcHandler::    CreateBucket(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlCreateBucket> &msg)
{
    const FdspCrtVolPtr crt_buck_req(new fpi::FDSP_CreateVolType());
    * crt_buck_req = msg->cv;
    LOGNOTIFY << "Received create bucket " << crt_buck_req->vol_name
              << " from  node uuid: "
              << std::hex << hdr->msg_src_uuid.svc_uuid << std::dec;

    try {
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        if (domain->om_local_domain_up()) {
            local->om_create_vol(nullptr, crt_buck_req, hdr);
        } else {
            LOGWARN << "OM Local Domain is not up yet, rejecting bucket "
                    << " create; try later";
        }
    }
    catch(...) {
        LOGERROR << "Orch Mgr encountered exception while "
                 << "processing create bucket";
    }
}

void
OmSvcHandler::    DeleteBucket(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlDeleteBucket> &msg)
{
    LOGNORMAL << " receive delete bucket from " << hdr->msg_src_uuid.svc_uuid;
    const FdspDelVolPtr del_buck_req(new fpi::FDSP_DeleteVolType());
    *del_buck_req = msg->dv;
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_delete_vol(nullptr, del_buck_req);
}

void
OmSvcHandler::    ModifyBucket(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlModifyBucket> &msg)
{
    LOGNORMAL << " receive delete bucket from " << hdr->msg_src_uuid.svc_uuid;
    const FdspModVolPtr mod_buck_req(new fpi::FDSP_ModifyVolType());
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_modify_vol(mod_buck_req);
}

void
OmSvcHandler::    PerfStats(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlPerfStats> &msg)
{
    extern OrchMgr *gl_orch_mgr;
    gl_orch_mgr->NotifyPerfstats(hdr, &msg->perfstats);
>>>>>>> dev
}

}  //  namespace fds

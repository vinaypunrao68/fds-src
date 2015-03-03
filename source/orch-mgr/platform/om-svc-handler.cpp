/*
 * Copyright 2013-2015 by Formations Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <iostream>  // NOLINT
#include <thrift/protocol/TDebugProtocol.h>
#include "om-svc-handler.h"
#include "OmResources.h"
#include "OmDeploy.h"
#include "OmDmtDeploy.h"
#include "OmDataPlacement.h"
#include "OmVolumePlacement.h"
#include "orch-mgr/om-service.h"
#include "util/Log.h"
#include "orchMgr.h"
#include "kvstore/redis.h"
#include "kvstore/configdb.h"
#include "net/net_utils.h"

namespace fds {

template<typename T>
static T
get_config(std::string const& option, T const& default_value)
{ 
    return MODULEPROVIDER()->get_fds_config()->get<T>(option, default_value); 
}

template <typename T, typename Cb>
static std::unique_ptr<TrackerBase<NodeUuid>>
create_tracker(Cb&& cb, std::string event, fds_uint32_t d_w = 0, fds_uint32_t d_t = 0) {
    static std::string const svc_event_prefix("fds.om.svc_event_threshold.");

    size_t window = get_config(svc_event_prefix + event + ".window", d_w);
    size_t threshold = get_config(svc_event_prefix + event + ".threshold", d_t);

    return std::unique_ptr<TrackerBase<NodeUuid>>
        (new TrackerMap<Cb, NodeUuid, T>(std::forward<Cb>(cb), window, threshold));
}

OmSvcHandler::~OmSvcHandler() {}

OmSvcHandler::OmSvcHandler(CommonModuleProviderIf *provider)
: PlatNetSvcHandler(provider)
{
    om_mod = OM_NodeDomainMod::om_local_domain();

    /* svc->om response message */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlTestBucket, TestBucket);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlCreateBucket, CreateBucket);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlDeleteBucket, DeleteBucket);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlModifyBucket, ModifyBucket);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlSvcEvent, SvcEvent);
//    REGISTER_FDSP_MSG_HANDLER(fpi::NodeInfoMsg, om_node_info);
//    REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, registerService);
    REGISTER_FDSP_MSG_HANDLER(fpi::GetSvcMapMsg, getSvcMap);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlTokenMigrationAbort, AbortTokenMigration);
}

int OmSvcHandler::mod_init(SysParams const *const param)
{
    this->configDB = gl_orch_mgr->getConfigDB();
    // TODO(bszmyd): Tue 20 Jan 2015 10:24:45 PM PST
    // This isn't probably where this should go, but for now it doesn't make
    // sense anymore for it to go anywhere else. When then dependencies are
    // better determined we should move this.
    // Register event trackers
    init_svc_event_handlers();
    return 0;
}

// Right now all handlers are using the same callable which will down the
// service that is responsible. This can be changed easily.
void OmSvcHandler::init_svc_event_handlers() {

    // callable for EventTracker. Changed from an anonymous function so I could
    // bind different errors to the same logic and retain the error for
    // om_service_down call.
    struct cb {
       void operator()(NodeUuid svc, size_t events) const {
           auto domain = OM_NodeDomainMod::om_local_domain();
           LOGERROR << std::hex << svc << " saw too many " << error << " events [" << events << "]";
           OM_NodeAgent::pointer agent = domain->om_all_agent(svc);

           if (agent) {
               agent->set_node_state(FDS_Node_Down);
               domain->om_service_down(error, svc, agent->om_agent_type());
           } else {
               LOGERROR << "unknown service: " << svc;
           }
       }
       Error               error;
    };

    // Timeout handler (2 within 15 minutes will trigger)
    event_tracker.register_event(ERR_SVC_REQUEST_TIMEOUT,
                                 create_tracker<std::chrono::minutes>(cb{ERR_SVC_REQUEST_TIMEOUT},
                                                                      "timeout", 15, 2));

    // DiskWrite handler (1 within 24 hours will trigger)
    event_tracker.register_event(ERR_DISK_WRITE_FAILED,
                                 create_tracker<std::chrono::hours>(cb{ERR_DISK_WRITE_FAILED},
                                                                    "disk.write_fail", 24, 1));

    // DiskRead handler (1 within 24 hours will trigger)
    event_tracker.register_event(ERR_DISK_READ_FAILED,
                                 create_tracker<std::chrono::hours>(cb{ERR_DISK_READ_FAILED},
                                                                    "disk.read_fail", 24, 1));
}

// om_svc_state_chg
// ----------------
//
void
OmSvcHandler::om_node_svc_info(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                               boost::shared_ptr<fpi::NodeSvcInfo> &svc)
{
    LOGNORMAL << "Node Service Info received for " << svc.get()->node_auto_name;
}

// om_node_info
// ------------
//
void
OmSvcHandler::om_node_info(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                           boost::shared_ptr<fpi::NodeInfoMsg> &node)
{
    LOGNORMAL << "Node Info received for " << node.get()->node_loc.svc_auto_name;
}

void
OmSvcHandler::TestBucket(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                 boost::shared_ptr<fpi::CtrlTestBucket> &msg)
{
    LOGNORMAL << " receive test bucket msg";
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_test_bucket(hdr, &msg->tbmsg);
}

void
OmSvcHandler::CreateBucket(boost::shared_ptr<fpi::AsyncHdr> &hdr,
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
OmSvcHandler::DeleteBucket(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                 boost::shared_ptr<fpi::CtrlDeleteBucket> &msg)
{
    LOGNORMAL << " receive delete bucket from " << hdr->msg_src_uuid.svc_uuid;
    const FdspDelVolPtr del_buck_req(new fpi::FDSP_DeleteVolType());
    *del_buck_req = msg->dv;
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_delete_vol(nullptr, del_buck_req);
}

void
OmSvcHandler::ModifyBucket(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                           boost::shared_ptr<fpi::CtrlModifyBucket> &msg)
{
    LOGNORMAL << " receive delete bucket from " << hdr->msg_src_uuid.svc_uuid;
    const FdspModVolPtr mod_buck_req(new fpi::FDSP_ModifyVolType());
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_modify_vol(mod_buck_req);
}

void
OmSvcHandler::SvcEvent(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                       boost::shared_ptr<fpi::CtrlSvcEvent> &msg)
{
    LOGDEBUG << " received " << msg->evt_code
             << " from:" << std::hex << msg->evt_src_svc_uuid.svc_uuid << std::dec;

    // XXX(bszmyd): Thu 22 Jan 2015 12:42:27 PM PST
    // Ignore timeouts from Om. These happen due to one-way messages
    // that cause Svc to timeout the request; e.g. TestBucket and SvcEvent
    if (gl_OmUuid == msg->evt_src_svc_uuid.svc_uuid &&
        ERR_SVC_REQUEST_TIMEOUT == msg->evt_code) {
        return;
    }
    event_tracker.feed_event(msg->evt_code, msg->evt_src_svc_uuid.svc_uuid);
}

void
OmSvcHandler::getSvcMap(boost::shared_ptr<fpi::AsyncHdr>&hdr,
                        boost::shared_ptr<fpi::GetSvcMapMsg> &msg)
{
    Error err(ERR_OK);
    LOGDEBUG << " received " << hdr->msg_code
             << " from:" << std::hex << hdr->msg_src_uuid.svc_uuid << std::dec;

    fpi::GetSvcMapRespMsgPtr respMsg (new fpi::GetSvcMapRespMsg());
    boost::shared_ptr<int64_t> nullarg;
    getSvcMap(respMsg->svcMap, nullarg);

    // initialize the response message with the service map
    hdr->msg_code = 0;
    sendAsyncResp (*hdr, FDSP_MSG_TYPEID(fpi::GetSvcMapRespMsg), *respMsg);
}

void OmSvcHandler::registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo)
{
    LOGDEBUG << "Register service request. Svcinfo: " << fds::logString(*svcInfo);

    /* TODO(OMteam): This registration should be handled in synchronized manner (single thread
     * handling is better) to avoid race conditions.
     */

    try
    {
        /* First update the service map */
        /* NOTE: Currently the new registation protocol is using existing code for node
         * registartion to reduce the amount of code churn.  Once they are unitfied, lot
         * of the ugly hacks below should go away
         */
        bool bResults = configDB->updateSvcMap(*svcInfo);
        if (!bResults)
        {
            return;
        }
        
        /* Convert new registration request to existing registration request */
        fpi::FDSP_RegisterNodeTypePtr reg_node_req;
        reg_node_req.reset(new FdspNodeReg());
        
        fromTo(svcInfo, reg_node_req);
        
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        NodeUuid new_node_uuid;

        if (reg_node_req->node_uuid.uuid == 0) {
            LOGERROR << "Refuse to register a node without valid uuid: node_type "
                << reg_node_req->node_type << ", name " << reg_node_req->node_name;
            return;
        }

        new_node_uuid.uuid_set_type(reg_node_req->node_uuid.uuid, 
                                    reg_node_req->node_type);
        /* Do the registration */
        Error err = domain->om_reg_node_info(new_node_uuid, reg_node_req);
        if (!err.ok()) 
        {
            LOGERROR << "Node Registration failed for "
                     << reg_node_req->node_name << ":" << std::hex
                     << new_node_uuid.uuid_get_val() << std::dec
                     << " node_type " << reg_node_req->node_type
                     << ", result: " << err.GetErrstr();
            return;
        }
        
        LOGNORMAL << "Done Registered new node " << reg_node_req->node_name 
                  << std::hex << ", node uuid " << reg_node_req->node_uuid.uuid
                  << ", service uuid " << new_node_uuid.uuid_get_val()
                  << ", node type " << reg_node_req->node_type << std::dec;
        
        // TODO(Paul/Rao): Broadcast the service map
    } 
    catch(const kvstore::ConfigException e)
    {
        LOGERROR << "Orchestration Manager encountered exception while "
                 << "processing register service -- "
                 << e.what()
                 << " SvcInfo: " << fds::logString(*svcInfo);
    }
    catch(const redis::RedisException e)
    {
        LOGERROR << "Orchestration Manager encountered exception while "
                 << "processing register service."
                 << " SvcInfo: " << fds::logString(*svcInfo);
    }
   
}

void OmSvcHandler::getSvcMap(std::vector<fpi::SvcInfo> & _return,
                       boost::shared_ptr<int64_t>& nullarg)
{
    LOGDEBUG << "Service map request";
    if (!configDB->getSvcMap(_return)) 
    {
        LOGWARN << "Failed to list service map";
    } 
}

void OmSvcHandler::setServiceProperty(boost::shared_ptr< fpi::SvcUuid>& svcUuid,
                                      boost::shared_ptr<std::string>& key,
                                      boost::shared_ptr<std::string>& value)
{
}

void OmSvcHandler::getServicePropery(std::string& _return,
                                     boost::shared_ptr< fpi::SvcUuid>& svcUuid,
                                     boost::shared_ptr<std::string>& key)
{
}

void OmSvcHandler::setServiceProperties(boost::shared_ptr< fpi::SvcUuid>& svcUuid,
                            boost::shared_ptr<std::map<std::string, std::string> >& props)
{
}

void OmSvcHandler::getServiceProperties(std::map<std::string, std::string> & _return,
                                        boost::shared_ptr< fpi::SvcUuid>& svcUuid)
{
}

void OmSvcHandler::AbortTokenMigration(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                  boost::shared_ptr<fpi::CtrlTokenMigrationAbort> &msg)
{
    LOGNORMAL << "Received abort token migration msg from "
              << std::hex << hdr->msg_src_uuid.svc_uuid << std::dec;
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();

    // tell DLT state machine about abort (error state)
    dltMod->dlt_deploy_event(DltErrorFoundEvt(NodeUuid(hdr->msg_src_uuid),
                                              Error(ERR_SM_TOK_MIGRATION_ABORTED)));
}

void OmSvcHandler::fromTo(boost::shared_ptr<fpi::SvcInfo>& svcInfo, 
                          fpi::FDSP_RegisterNodeTypePtr& reg_node_req)
{       
       
//    FDS_ProtocolInterface::FDSP_AnnounceDiskCapability& capacity;
//    capacity = new FDS_ProtocolInterface::FDSP_AnnounceDiskCapability();
//    reg_node_req->disk_info = capacity;
//    reg_node_req->domain_id = 0;
//    reg_node_req->ip_hi_addr = 0;
//    reg_node_req->metasync_port = 0;
//    reg_node_req->migration_port = 0;
//    reg_node_req->node_root = new std::string();    
//    reg_node_req->node_uuid = new FDSP_Uuid(); 
    
    reg_node_req->control_port = svcInfo->svc_port;
    reg_node_req->data_port = svcInfo->svc_port;
    reg_node_req->ip_lo_addr = fds::net::ipString2Addr(svcInfo->ip);  
    reg_node_req->node_name = svcInfo->name;   
    reg_node_req->node_type = svcInfo->svc_type;
    
   
    FDS_ProtocolInterface::FDSP_Uuid uuid;
    reg_node_req->service_uuid = fds::assign(uuid, svcInfo->svc_id);
    
}

}  //  namespace fds

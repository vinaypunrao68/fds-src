/*
 *
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
#include <net/SvcMgr.h>
#include <ctime>
#include <omutils.h>
#include <fdsp_utils.h>

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

    LOGNORMAL << "Setting event " << event << " handling threshold to " << threshold;

    return std::unique_ptr<TrackerBase<NodeUuid>>
        (new TrackerMap<Cb, NodeUuid, T>(std::forward<Cb>(cb), window, threshold));
}

OmSvcHandler::~OmSvcHandler() {}

OmSvcHandler::OmSvcHandler(CommonModuleProviderIf *provider)
: PlatNetSvcHandler(provider)
{
    om_mod = OM_NodeDomainMod::om_local_domain();

    /* svc->om response message */
    REGISTER_FDSP_MSG_HANDLER(fpi::GetVolumeDescriptor, getVolumeDescriptor);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlSvcEvent, SvcEvent);
//    REGISTER_FDSP_MSG_HANDLER(fpi::NodeInfoMsg, om_node_info);
//    REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, registerService);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlTokenMigrationAbort, AbortTokenMigration);
    REGISTER_FDSP_MSG_HANDLER(fpi::NotifyHealthReport, notifyServiceRestart);
    REGISTER_FDSP_MSG_HANDLER(fpi::HeartbeatMessage, heartbeatCheck);
    REGISTER_FDSP_MSG_HANDLER(fpi::SvcStateChangeResp, svcStateChangeResp);
    REGISTER_FDSP_MSG_HANDLER(fpi::SetVolumeGroupCoordinatorMsg, setVolumeGroupCoordinator);
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
           OM_NodeAgent::pointer agent = domain->om_all_agent(svc);

           if (agent) {

               /*
                * FS-1424 P. Tinius 03/24/2015
                * No need to spam the log with the log errors, when the
                * real issue
                */
               LOGERROR << std::hex << svc << " " << agent->node_get_svc_type()
                        << " saw too many " << std::dec << error
                        << " events [" << events << "]";

               agent->set_node_state(fpi::FDS_Node_Down);
               domain->om_service_down(error, svc, agent->node_get_svc_type());
           } else {

               LOGERROR << "unknown service: " << svc;

           }
       }
       Error               error;
    };

    // Timeout handler (1 within 15 minutes will trigger)
    event_tracker.register_event(ERR_SVC_REQUEST_TIMEOUT,
                                 create_tracker<std::chrono::minutes>(cb{ERR_SVC_REQUEST_TIMEOUT},
                                                                      "timeout", 15, 1));

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
OmSvcHandler::getVolumeDescriptor(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                 boost::shared_ptr<fpi::GetVolumeDescriptor> &msg)
{
    LOGNORMAL << " receive getVolumeDescriptor msg";
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    auto resp = fpi::GetVolumeDescriptorResp();
    VolumeDesc desc(msg->volume_name, invalid_vol_id);
    auto err = local->om_get_volume_descriptor(hdr, msg->volume_name, desc);
    if (ERR_OK == err) {
        desc.toFdspDesc(resp.vol_desc);
    }
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::GetVolumeDescriptorResp), resp);
}

void
OmSvcHandler::SvcEvent(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                       boost::shared_ptr<fpi::CtrlSvcEvent> &msg)
{

    // XXX(bszmyd): Thu 22 Jan 2015 12:42:27 PM PST
    // Ignore timeouts from Om. These happen due to one-way messages
    // that cause Svc to timeout the request; e.g. GetVolumeDescriptor and SvcEvent
    if (gl_OmUuid == msg->evt_src_svc_uuid.svc_uuid &&
        ERR_SVC_REQUEST_TIMEOUT == msg->evt_code) {
        return;
    }

    /*
     * FS-1424 P. Tinius 03/24/2015
     * Move this log message to after the filtering check, no need to spam
     * the log for filtered out events.
     */
    LOGDEBUG << " received " << msg->evt_code
             << " from:" << std::hex << msg->evt_src_svc_uuid.svc_uuid << std::dec;
    event_tracker.feed_event(msg->evt_code, msg->evt_src_svc_uuid.svc_uuid);
}

void OmSvcHandler::registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo)
{
    LOGDEBUG << "Register service request. Svcinfo: " << fds::logString(*svcInfo);
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    Error err = domain->om_register_service(svcInfo);
    if (!err.ok()) {
        LOGERROR << logString(*svcInfo) << err;
        throw fpi::OmRegisterException();
    }
}

/**
 * Allows the pulling of the DLT. Returns DLT_VER_INVALID if there's no committed DLT yet.
 */

void OmSvcHandler::getDLT( ::FDS_ProtocolInterface::CtrlNotifyDLTUpdate& dlt, boost::shared_ptr<int64_t>& nullarg) {
	OM_Module *om = OM_Module::om_singleton();
	DataPlacement *dp = om->om_dataplace_mod();
	std::string data_buffer;
	DLT const *dtp = nullptr;
    FDS_ProtocolInterface::FDSP_DLT_Data_Type dlt_val;
	if (!(dp->getCommitedDlt())){
		LOGDEBUG << "Not sending DLT to new node, because no "
                << " committed DLT yet";
        dlt.__set_dlt_version(DLT_VER_INVALID);

	} else {
		LOGDEBUG << "Should have DLT to send";
		dtp = dp->getCommitedDlt();
		dtp->getSerialized(data_buffer);
		dlt.__set_dlt_version(dp->getCommitedDltVersion());
		dlt_val.__set_dlt_data(data_buffer);
		dlt.__set_dlt_data(dlt_val);
	}
}

/**
 * Allows the pulling of the DMT. Returns DMT_VER_INVALID if there's no committed DMT yet.
 */

void OmSvcHandler::getDMT( ::FDS_ProtocolInterface::CtrlNotifyDMTUpdate& dmt, boost::shared_ptr<int64_t>& nullarg) {
	OM_Module *om = OM_Module::om_singleton();
	VolumePlacement* vp = om->om_volplace_mod();
	std::string data_buffer;
    if (vp->hasCommittedDMT()) {
    	DMTPtr dp = vp->getCommittedDMT();
    	LOGDEBUG << "Should have DMT to send";
    	(*dp).getSerialized(data_buffer);

    	::FDS_ProtocolInterface::FDSP_DMT_Data_Type fdt;
    	fdt.__set_dmt_data(data_buffer);

    	dmt.__set_dmt_version(vp->getCommittedDMTVersion());
    	dmt.__set_dmt_data(fdt);
    } else {
        LOGDEBUG << "Not sending DMT to new node, because no "
                << " committed DMT yet";
        dmt.__set_dmt_version(DMT_VER_INVALID);
    }
}

void OmSvcHandler::getSvcInfo(fpi::SvcInfo &_return,
                              boost::shared_ptr< fpi::SvcUuid>& svcUuid)
{
    bool ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(*svcUuid, _return);
    if (!ret) {
        LOGWARN << "Failed to lookup svcuuid: " << svcUuid->svc_uuid;
        throw fpi::SvcLookupException();
    }
}

static void
populate_voldesc_list(fpi::GetAllVolumeDescriptors &list, VolumeInfo::pointer vol)
{
	TRACEFUNC;
	if (vol->isDeletePending() || vol->isStateDeleted()) {
		LOGDEBUG << "Not sending volume " << vol->vol_get_name() << " due to deletion.";
		return;
	}
	list.volumeList.emplace_back();
	auto &volAdd = list.volumeList.back();
	vol->vol_get_properties()->toFdspDesc(volAdd.vol_desc);
	LOGDEBUG << "Populated list with volume " << volAdd.vol_desc.vol_name;

}

void OmSvcHandler::getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors& _return, boost::shared_ptr<int64_t> &nullarg) {
    LOGDEBUG << "Received get all volume descriptors";

	OM_Module *om = OM_Module::om_singleton();
	OM_NodeDomainMod *dom_mod = om->om_nodedomain_mod();
	OM_NodeContainer *local = dom_mod->om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();

    // First clear all the vol descriptors in the return list and
    // then populate the list one by one
    _return.volumeList.clear();

    volumes->vol_foreach<fpi::GetAllVolumeDescriptors&>(_return, populate_voldesc_list);
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
/*
 * This will handle the heartbeatMessage coming from the PM
 * */
void OmSvcHandler::heartbeatCheck(boost::shared_ptr<fpi::AsyncHdr>& hdr,
                                  boost::shared_ptr<fpi::HeartbeatMessage>& msg)
{
    fpi::SvcUuid svcUuid;
    svcUuid.svc_uuid = msg->svcUuid.uuid;

    auto curTimePoint = std::chrono::system_clock::now();
    std::time_t time  = std::chrono::system_clock::to_time_t(curTimePoint);

    LOGDEBUG << "Received heartbeat from PM:"
             << std::hex << svcUuid.svc_uuid <<std::dec;

    // Get the time since epoch and convert it to minutes
    auto timeSinceEpoch = curTimePoint.time_since_epoch();
    double current      = std::chrono::duration<double,std::ratio<60>>
                                       (timeSinceEpoch).count();

    bool updSvcState = false;
    if ( !gl_orch_mgr->omMonitor->isWellKnown(svcUuid) ) {
        updSvcState = true;
    }

    configDB->setCapacityUsedNode( svcUuid.svc_uuid, msg->usedCapacityInBytes );

    gl_orch_mgr->omMonitor->updateKnownPMsMap(svcUuid, current, updSvcState);
}

void OmSvcHandler::svcStateChangeResp(boost::shared_ptr<fpi::AsyncHdr>& hdr,
                                      boost::shared_ptr<fpi::SvcStateChangeResp>& msg)
{
    LOGDEBUG << "Received state change response from PM:"
             << std::hex << msg->pmSvcUuid.svc_uuid << std::dec
             << " for start request";

    NodeUuid node_uuid(msg->pmSvcUuid);

    int64_t uuid = msg->pmSvcUuid.svc_uuid;

    if (lastHeardResp.first == 0) {
        // This is the very first start resp in OM's history
        lastHeardResp = std::make_pair(uuid, fds::util::getTimeStampSeconds());

    } else if (lastHeardResp.first == uuid) {
        int32_t current     = fds::util::getTimeStampSeconds();
        int32_t elapsedSecs = current - lastHeardResp.second;

        // If we are receiving a resp from the same PM within a second, ignore the response.
        if (elapsedSecs <= 1) {

            LOGDEBUG << "Received response from the same PM less than a second ago, will ignore";
            lastHeardResp.second = current;
            return;
        }
        lastHeardResp.second = current;

    } else {
        // Update to hold the latest response received
        lastHeardResp = std::make_pair(uuid, fds::util::getTimeStampSeconds());
    }

    auto item = std::make_pair(node_uuid.uuid_get_val(), 0);
    gl_orch_mgr->removeFromSentQ(item);

    OM_PmAgent::pointer agent = OM_Module::om_singleton()->om_nodedomain_mod()->
            om_loc_domain_ctrl()->om_pm_agent(node_uuid);

    agent->send_start_service_resp(msg->pmSvcUuid, msg->changeList);
}

void OmSvcHandler::notifyServiceRestart(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                        boost::shared_ptr<fpi::NotifyHealthReport> &msg)
{
    LOGNORMAL << "Received Health Report: "
              << msg->healthReport.serviceInfo.svc_id.svc_name
              << " state: " << msg->healthReport.serviceState
              << " status: " << msg->healthReport.statusCode 
              << " uuid: " 
              << std::hex 
              << msg->healthReport.serviceInfo.svc_id.svc_uuid.svc_uuid
              << std::dec;

    ResourceUUID service_UUID (msg->healthReport.serviceInfo.svc_id.svc_uuid.svc_uuid);
    fpi::FDSP_MgrIdType service_type = service_UUID.uuid_get_type();
    fpi::FDSP_MgrIdType comp_type = fpi::FDSP_INVALID_SVC;

    switch (msg->healthReport.serviceState) {
        case fpi::HEALTH_STATE_RUNNING:
            healthReportRunning( msg );
            break;
        case fpi::HEALTH_STATE_INITIALIZING:
        case fpi::HEALTH_STATE_DEGRADED:
        case fpi::HEALTH_STATE_LIMITED:
        case fpi::HEALTH_STATE_SHUTTING_DOWN:
            LOGWARN << "Handling for service " << msg->healthReport.serviceInfo.name
                    << " state: "
                    << msg->healthReport.serviceState
                    << " uuid: "
                    << std::hex
                    << msg->healthReport.serviceInfo.svc_id.svc_uuid.svc_uuid
                    << std::dec
                    << " -- not implemented yet.";
            break;
        case fpi::HEALTH_STATE_ERROR:
            healthReportError(service_type, msg);
            break;
        case fpi::HEALTH_STATE_UNREACHABLE:
            LOGERROR << "Handling unreachable event for service " 
                     << msg->healthReport.serviceInfo.name
                     << " in state " 
                     << msg->healthReport.serviceState;

            healthReportUnreachable( service_type, msg );

            // Track this error event as a timeout. We're assuming a timeout is
            // indistinguishable from a service being unreachable.
            event_tracker.feed_event(ERR_SVC_REQUEST_TIMEOUT, service_UUID);
            break;
        case fpi::HEALTH_STATE_UNEXPECTED_EXIT:
            // Generally dispatched by PM when it sees a service's process abort unexpectedly
            switch (service_type) {
                case fpi::FDSP_ACCESS_MGR:
                    comp_type = (comp_type == fpi::FDSP_INVALID_SVC) ? fpi::FDSP_ACCESS_MGR : comp_type;
                    // no break
                case fpi::FDSP_DATA_MGR:
                    comp_type = (comp_type == fpi::FDSP_INVALID_SVC) ? fpi::FDSP_DATA_MGR : comp_type;
                    // no break
                case fpi::FDSP_STOR_MGR: {
                    comp_type = (comp_type == fpi::FDSP_INVALID_SVC) ? fpi::FDSP_STOR_MGR : comp_type;
                    healthReportUnexpectedExit(comp_type, msg);
                    break;
                }
                default:
                    LOGERROR << "Unhandled process: "
                             << msg->healthReport.serviceInfo.svc_id.svc_name.c_str()
                             << " with service type "
                             << service_type;
                    break;
            }
            break;
        case fpi::HEALTH_STATE_FLAPPING_DETECTED_EXIT:
            // TODO raise alarm/event
            healthReportError( service_type, msg );
            break;

        default:
            LOGERROR << "Unknown service state: %d", msg->healthReport.serviceState;
            break;
    }
}

void OmSvcHandler::healthReportRunning( boost::shared_ptr<fpi::NotifyHealthReport> &msg )
{
   LOGDEBUG << "Service Running health report";

   ResourceUUID service_UUID (msg->healthReport.serviceInfo.svc_id.svc_uuid.svc_uuid);
   fpi::FDSP_MgrIdType service_type = service_UUID.uuid_get_type();
   fpi::FDSP_MgrIdType comp_type = fpi::FDSP_INVALID_SVC;

   switch (service_type)
   {
     case fpi::FDSP_ACCESS_MGR:
       comp_type = (comp_type == fpi::FDSP_INVALID_SVC) ? fpi::FDSP_ACCESS_MGR : comp_type;
       // no break
     case fpi::FDSP_DATA_MGR:
       comp_type = (comp_type == fpi::FDSP_INVALID_SVC) ? fpi::FDSP_DATA_MGR : comp_type;
       // no break
     case fpi::FDSP_STOR_MGR:
       comp_type = (comp_type == fpi::FDSP_INVALID_SVC) ? fpi::FDSP_STOR_MGR : comp_type;

       if ( isSameSvcInfoInstance( msg->healthReport.serviceInfo ) )
       {
           NodeUuid uuid( msg->healthReport.serviceInfo.svc_id.svc_uuid.svc_uuid );
           auto domain = OM_NodeDomainMod::om_local_domain();

           LOGNORMAL << "Will set service to state ( "
                     << msg->healthReport.serviceInfo.svc_status
                     << " ) : " << msg->healthReport.serviceInfo.name
                     << ":0x" << std::hex << uuid.uuid_get_val() << std::dec;

           domain->om_change_svc_state_and_bcast_svcmap( uuid, service_type, msg->healthReport.serviceInfo.svc_status );
           domain->om_service_up( uuid, service_type );
       }
       break;
     default:
       LOGDEBUG << "unimplemented health report running service: "
     	          << msg->healthReport.serviceInfo.svc_id.svc_name.c_str()
     	          << " with service type "
     		        << service_type;
       break;
   }

   /*
    * It is expected that if a service restarts, it will re-register with the
    * OM. Which should update all the appropriate service dependencies.
    *
    * So I don't believe that is anything else to do here.
    */
}

void OmSvcHandler::healthReportUnexpectedExit(fpi::FDSP_MgrIdType &comp_type,
		boost::shared_ptr<fpi::NotifyHealthReport> &msg) {
	/**
	 * When a PM pings this OM with the state of an individual service
	 * restart (AM/DM/SM) that the PM originally spawned, then we will
	 * find the AM/DM/SM agent that corresponds to the report, and use the
	 * PM method to deactivate the AM/DM/SM that is currently registered
	 * with the OM.
	 * It should then reactivate because user is expecting the service to
	 * be up since it's passed in as UNEXPECTED.
	 */
	 LOGNORMAL << "Cleaning up old process information.";
	 std::list<fds::NodeSvcEntity> pm_services;
 	 bool pm_found = false;
	 NodeSvcEntity *actual_pm_service = nullptr;
	 OM_PmContainer::pointer pm_nodes = OM_Module::om_singleton()->
								om_nodedomain_mod()-> om_loc_domain_ctrl()->om_pm_nodes();
	 pm_nodes->populate_nodes_in_container(pm_services);

	 /**
	  * We need to find the right PM agent who is in charge of the service
	  * that just died.
	  */
	 for (std::list<NodeSvcEntity>::iterator i = pm_services.begin();
			i != pm_services.end(); i++) {
		 // Technically node_uuid should be unsigned too...
		 if (i->node_uuid.uuid_get_val() ==
			(fds_uint64_t)(msg->healthReport.platformUUID.svc_uuid.svc_uuid)) {
			pm_found = true;
			actual_pm_service = &(*i);
			break;
		 }
	 }

   if ( pm_found )
   {
     OM_PmAgent::pointer om_pm_agt = OM_Module::om_singleton()->om_nodedomain_mod()->
        om_loc_domain_ctrl()->om_pm_agent(actual_pm_service->node_uuid);
     /**
      * PM doesn't want to hear about the actual deactivate service but
      * OM side needs to deactivate it. So just call the response
      * which will do the OM side cleanup.
      */
     Error dummyErr (ERR_OK);
     om_pm_agt->send_deactivate_services_resp(comp_type == fpi::FDSP_STOR_MGR,
        comp_type == fpi::FDSP_DATA_MGR,
        comp_type == fpi::FDSP_ACCESS_MGR,
        nullptr, dummyErr, nullptr);
	 }
}

void OmSvcHandler::healthReportUnreachable( fpi::FDSP_MgrIdType &svc_type,
                                            boost::shared_ptr<fpi::NotifyHealthReport> &msg) 
{
    // we only handle specific errors from SM and DM for now
    if ( ( svc_type == fpi::FDSP_STOR_MGR ) || ( svc_type == fpi::FDSP_DATA_MGR ) ) {
        /*
         * if unreachable service incarnation is the same as the service map, change the state to INVALID
        */
        if ( isSameSvcInfoInstance( msg->healthReport.serviceInfo ) )
        {
            if ( (msg->healthReport.serviceInfo.svc_status == fpi::SVC_STATUS_REMOVED) &&
                 ((svc_type == fpi::FDSP_STOR_MGR) || (svc_type == fpi::FDSP_DATA_MGR)) ) {

                // It is important that SMs and DMs stay in removed state for correct
                // handling if interruptions occur before commit of the DLT or DMT.
                // If the svc is in REMOVED state, it has been stopped and is already INACTIVE
                LOGDEBUG << "Service:" << std::hex << msg->healthReport.serviceInfo.svc_id.svc_uuid.svc_uuid
                         << std::dec << " in REMOVED state, will not change state to INACTIVE";
                return;
            }

            auto domain = OM_NodeDomainMod::om_local_domain();
            NodeUuid uuid(msg->healthReport.serviceInfo.svc_id.svc_uuid.svc_uuid);
            Error reportError(msg->healthReport.statusCode);

            LOGERROR << "Will set service to failed state: "
            << msg->healthReport.serviceInfo.name
            << ":0x" << std::hex << uuid.uuid_get_val() << std::dec;

            OM_NodeAgent::pointer agent = domain->om_all_agent(uuid);
            if (agent) {
                /*
                 *  required so that DMT/DLT will see it as a failed service
                 */
                agent->set_node_state( fpi::FDS_Node_Down );
            }

            /*
             * change the state and update service map; then broadcast updated service map
             */
            // don't mark this to inactive failed if it is already in stopped state
            if (gl_orch_mgr->getConfigDB()->getStateSvcMap(uuid.uuid_get_val()) != fpi::SVC_STATUS_INACTIVE_STOPPED)
            {
                domain->om_change_svc_state_and_bcast_svcmap( uuid, svc_type, fpi::SVC_STATUS_INACTIVE_FAILED );
            } else {
                LOGWARN << "Svc:" << std::hex << uuid.uuid_get_val() << std::dec
                        << "has been set to inactive from a previous stop request";
            }
            domain->om_service_down( reportError, uuid, svc_type );
        }

        return;
    }
}

void OmSvcHandler::healthReportError(fpi::FDSP_MgrIdType &svc_type,
                                     boost::shared_ptr<fpi::NotifyHealthReport> &msg) {
    Error reportError(msg->healthReport.statusCode);

    // we only handle specific errors from SM and DM for now
    if ((svc_type == fpi::FDSP_STOR_MGR) ||
        (svc_type == fpi::FDSP_DATA_MGR)) {
        if ((reportError == ERR_SERVICE_CAPACITY_FULL) ||
            // TODO: we should implement finer-grained handling here
            // This error means that SM failed to sync one or more DLT tokens
            // for which this is primary; SM currently reports ERROR to OM so that
            // we set it to failed state to make OM rotate DLT and make this SM
            // secondary for its DLT tokens. In the future, SM should be able
            // to report specific DLT tokens that failed and OM should rotate only
            // those DLT columns; but we need to implement this in both state machine
            // and DLT computation module.
            (reportError == ERR_TOKEN_NOT_READY) ||
            // service is unavailable -- most likely failed to initialize
            (reportError == ERR_NODE_NOT_ACTIVE)) {
            // when a service reports these errors, it sets itself inactive
            // so OM should also set service state to failed and update
            // DLT/DMT accordingly
            auto domain = OM_NodeDomainMod::om_local_domain();
            NodeUuid uuid(msg->healthReport.serviceInfo.svc_id.svc_uuid.svc_uuid);
            OM_NodeAgent::pointer agent = domain->om_all_agent(uuid);
            if (agent) {
                LOGDEBUG << "Will set service to failed state: "
                         << msg->healthReport.serviceInfo.name
                         << ":0x" << std::hex << uuid.uuid_get_val() << std::dec;
                agent->set_node_state(fpi::FDS_Node_Down);
                domain->om_service_down(reportError, uuid, agent->node_get_svc_type());
            } else {
                LOGDEBUG << "Got error health report from service "
                         << msg->healthReport.serviceInfo.svc_id.svc_name
                         << ":0x" << std::hex << uuid.uuid_get_val() << std::dec
                         << " that OM does not know about; ignoring";
            }
            return;
        }
    }

    if ( msg->healthReport.serviceState == fpi::HEALTH_STATE_FLAPPING_DETECTED_EXIT )
    {
        auto domain = OM_NodeDomainMod::om_local_domain();
        NodeUuid uuid(msg->healthReport.serviceInfo.svc_id.svc_uuid.svc_uuid);

        FdsTimerTaskPtr task;

        if (domain->isScheduled(task, uuid.uuid_get_val()))
        {
            LOGNOTIFY << "Canceling scheduled setupNewNode task for svc:"
                      << std::hex << uuid.uuid_get_val() << std::dec;
            MODULEPROVIDER()->getTimer()->cancel(task);
        }

        fpi::ServiceStatus status = gl_orch_mgr->getConfigDB()->getStateSvcMap(uuid.uuid_get_val());

        // PM can send this error either when OM told it to start the service
        // or if a service has restarted too many times (but the svc is registered already)
        if ( status == fpi::SVC_STATUS_STARTED || status == fpi::SVC_STATUS_ACTIVE)
        {
            LOGNOTIFY << "Received Flapping error from PM for service:"
                      << std::hex << uuid.uuid_get_val() << std::dec
                      << " , setting to state INACTIVE_FAILED";
            domain->om_change_svc_state_and_bcast_svcmap( uuid, svc_type, fpi::SVC_STATUS_INACTIVE_FAILED );

        } else if (status == fpi::SVC_STATUS_INACTIVE_FAILED) {
            LOGNOTIFY << "Flapping service:"<< std::hex << uuid.uuid_get_val() << std::dec
                      << " is already in INACTIVE_FAILED state";
        } else {
            LOGWARN << "Received Flapping error from PM for service:"
                      << std::hex << uuid.uuid_get_val() << std::dec
                      << " , ignoring since svc is not in started/active state, current state:" << status;
        }

        return;
    }

    // if we are here, we don't handle the error and/or service yet
    LOGWARN << "Handling ERROR report for service " << msg->healthReport.serviceInfo.name
            << " state: " << msg->healthReport.serviceState
            << " error: " << msg->healthReport.statusCode << " not implemented yet.";
}

void
OmSvcHandler::setVolumeGroupCoordinator(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                        boost::shared_ptr<fpi::SetVolumeGroupCoordinatorMsg> &msg)
{
    /* xxx: This needs to be synchronized and be done volume managing statemachine */
    Error e;
    fds_volid_t volId(msg->volumeId);
	OM_Module *om = OM_Module::om_singleton();
	OM_NodeDomainMod *dom_mod = om->om_nodedomain_mod();
	OM_NodeContainer *local = dom_mod->om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();

    auto volumePtr = volumes->get_volume(volId);
    if (volumePtr != nullptr) {
        auto volDescPtr = volumePtr->vol_get_properties();
        fpi::VolumeGroupCoordinatorInfo volCoordinatorInfo = msg->coordinator;
        volDescPtr->setCoordinatorId(volCoordinatorInfo.id);
        volDescPtr->setCoordinatorVersion(volCoordinatorInfo.version);
    } else {
        LOGERROR << "Unable to find volume " << volId;
        e = ERR_VOL_NOT_FOUND;
    }

    hdr->msg_code = e.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
}


}  //  namespace fds

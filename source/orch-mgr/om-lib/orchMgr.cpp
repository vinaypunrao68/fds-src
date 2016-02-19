/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>

#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>
#include <OmResources.h>
#include <lib/Catalog.h>
#include <map>
#include <util/Log.h>
#include <OmDataPlacement.h>
#include <OmVolumePlacement.h>
#include <orch-mgr/om-service.h>
#include <fdsp/OMSvc.h>
#include "fdsp/common_constants.h"
#include <om-svc-handler.h>
#include <net/SvcMgr.h>

namespace fds {

OrchMgr *orchMgr;

OrchMgr::OrchMgr(int argc, char *argv[], OM_Module *omModule, bool initAsModule, bool testMode)
    : conf_port_num(0),
      ctrl_port_num(0),
      test_mode(testMode),
      deleteScheduler(this),
      enableTimeline(true)
{
    om_mutex = new fds_mutex("OrchMgrMutex");
    fds::gl_orch_mgr = this;
    orchMgr = this;

    static fds::Module *omVec[] = {
        omModule,
        NULL
    };

    for (int i = 0; i < MAX_OM_NODES; i++) {
        /*
         * TODO: Make this variable length rather
         * that statically allocated.
         */
        node_id_to_name[i] = "";
    }

    // Note on Thrift service compatibility:
    // If a backward incompatible change arises, pass additional pairs of
    // processor and Thrift service name to SvcProcess::init(). Similarly,
    // if the Thrift service API wants to be broken up.
    init<fds::OmSvcHandler, fpi::OMSvcProcessor>(argc, argv, initAsModule, "platform.conf",
        "fds.om.", "om.log", omVec, fpi::commonConstants().OM_SERVICE_NAME);

    enableTimeline = get_fds_config()->get<bool>(
            "fds.feature_toggle.common.enable_timeline", true);
    counters.reset(new fds::om::Counters(get_cntrs_mgr().get()));

    if (!test_mode && enableTimeline) {
        snapshotMgr.reset(new fds::snapshot::Manager(this));
    }

    /*
     * Start the PM monitoring thread
     */
    if (!test_mode) {
        omMonitor.reset(new OMMonitorWellKnownPMs());
        svcStartThread.reset(new std::thread(&OrchMgr::svcStartMonitor, this));
        svcStartThread->detach();
        svcStartRetryThread.reset(new std::thread(&OrchMgr::svcStartRetryMonitor, this));
        svcStartRetryThread->detach();
    } else {
        enableTimeline = false;
    }
    /*
     * Testing code for loading test info from disk.
     */
    LOGDEBUG << "Constructor of Orchestration Manager ( called )";
}

OrchMgr::~OrchMgr()
{
    LOGDEBUG << "Destructor for Orchestration Manager ( called )";

    if (!test_mode) {
        cfgserver_thread->join();
    }

    if (policy_mgr) {
        delete policy_mgr;
    }
    fds::gl_orch_mgr =  nullptr;
}

void OrchMgr::proc_pre_startup()
{
    int    argc;
    char **argv;

    argv = mod_vectors_->mod_argv(&argc);
    
    /*
     * Process the cmdline args.
     */
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--port=", 7) == 0) {
            conf_port_num = strtoul(argv[i] + 7, NULL, 0);
        } else if (strncmp(argv[i], "--cport=", 8) == 0) {
            ctrl_port_num = strtoul(argv[i] + 8, NULL, 0);
        } else if (strncmp(argv[i], "--prefix=", 9) == 0) {
            stor_prefix = argv[i] + 9;
        } else if (strncmp(argv[i], "--test", 7) == 0) {
            test_mode = true;
        }
    }

    my_node_name = stor_prefix + std::string("OrchMgr");

    std::string ip_address;
    int config_portnum;
    int control_portnum;
    if (conf_port_num != 0) {
        config_portnum = conf_port_num;
    } else {
        config_portnum = conf_helper_.get<int>("config_port");
    }
    if (ctrl_port_num != 0) {
        control_portnum = ctrl_port_num;
    } else {
        // platform + 4 is the default, but use fds.common.om_port rather than
        // SvcMgr::mapToSvcPort
        auto platformPort = conf_helper_.get_abs<int>("fds.pm.platform_port");
        auto svcType = SvcMgr::mapToSvcType(svcName_);
        control_portnum = conf_helper_.get_abs<int>("fds.common.om_port",
                                                    SvcMgr::mapToSvcPort(platformPort, svcType));
    }
    
    LOGDEBUG << "Orchestration Manager using config port " << config_portnum
             << " control port " << control_portnum;

    policy_mgr = new VolPolicyMgr(getConfigDB(), GetLog());

    defaultS3BucketPolicy();

    cfgserver_thread.reset(new std::thread(&OrchMgr::start_cfgpath_server, this));
        
    LOGDEBUG << "Orchestration Manager is starting service layer";
    SvcProcess::proc_pre_startup();
}

void OrchMgr::proc_pre_service()
{
    if ( !test_mode && enableTimeline )
    {
        snapshotMgr->init();
    }
    
    fds_bool_t config_db_up = loadFromConfigDB();
    
    // load persistent state to local domain
    OM_NodeDomainMod* local_domain = OM_NodeDomainMod::om_local_domain();
    local_domain->om_load_state(config_db_up ? getConfigDB() : NULL);
}

void OrchMgr::setupConfigDb_()
{
    /*
     * no need to call this, service layer doesn't need to do any configdb
     * setup. We should remove this from service layer 05/22/2015 (Tinius )
     */
//    SvcProcess::setupConfigDb_();
    
    configDB = new kvstore::ConfigDB(
        conf_helper_.get<std::string>( "configdb.host", "localhost" ),
        conf_helper_.get<int>( "configdb.port", 0 ),
        conf_helper_.get<int>( "configdb.poolsize", 10 ) );
        
    LOGDEBUG << "ConfigDB Initialized";
}

void OrchMgr::setupSvcInfo_()
{
    /*
     * without this we see the following error during startup
     * '/fds/bin/liborchmgr.so: /fds/bin/liborchmgr.so: undefined symbol: 
     * _ZTVN3fds7OrchMgrE'
     */
    SvcProcess::setupSvcInfo_();
}

void OrchMgr::registerSvcProcess()
{
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    LOGDEBUG << "Registering OM service: " << fds::logString( svcInfo_ )
             << " LOCAL DOMAIN UP " << domain->om_local_domain_up()
             << " DOWN " << domain->om_local_domain_down();

    /* Add om information to service map */
    svcMgr_->updateSvcMap({svcInfo_});
}

int OrchMgr::run()
{
    // At this point, all module has init and started. So signal the waiters.
    readyWaiter.done();
    // run server to listen for OMControl messages from
    // SM, DM and SH
    if (!test_mode) {
        deleteScheduler.start();
        runConfigService(this);
    } else {
        // just sleep for the duration of the test
        while (true) {
            sleep(60);
        }
    }
    return 0;
}

void OrchMgr::svcStartMonitor()
{
    while (true) {

        std::unique_lock<std::mutex> toSendQLock(toSendQMutex);
        toSendQCondition.wait(toSendQLock, [this] { return !toSendMsgQueue.empty(); });

        LOGDEBUG << "Services start monitor is running...";

        NodeUuid node_uuid;
        bool domainRestart       = false;
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();

        PmMsg msg    = toSendMsgQueue.back();
        int64_t uuid = msg.first;

        constructMsgParams(uuid, node_uuid, domainRestart);

        bool issuedStart = domain->om_activate_known_services(domainRestart, node_uuid );

        // Remove from the toSend queue
        toSendMsgQueue.pop_back();

        if ( issuedStart )
        {
            // Unlock here since we will acquire sentQ lock further
            // down, best to avoid holding a lock within a lock
            // When we loop back, the wait will reacquire toSendQLock
            // if needed
            toSendQLock.unlock();
            // Update the timestamp on the message
            msg.second = util::getTimeStampSeconds();

            // Add to the sentQueue
            addToSentQ(msg);
        } else {
            LOGWARN << "PM:" << std::hex << uuid << std::dec
                    << " removed from all start queues, no eligible svcs to start";
        }
    }
}

void OrchMgr::svcStartRetryMonitor()
{
    while (true) {

        std::unique_lock<std::mutex> sentQLock(sentQMutex);
        sentQCondition.wait(sentQLock, [this] { return !sentMsgQueue.empty(); });

        LOGDEBUG << "Services start retry monitor is running...";
        int32_t current = util::getTimeStampSeconds();
        bool foundRetry = false;
        PmMsg retryMsg;
        int32_t timeElapsed = 0;

        // SECTION: Calculate if a retry is required
        for (auto item : sentMsgQueue) {

            timeElapsed = current - item.second;

            LOGDEBUG << "Service start msg sent to PM uuid:"
                     << std::hex << item.first << std::dec
                     << "  " << timeElapsed << "seconds ago";

            if ( timeElapsed >= 3 ) {
                foundRetry = true;
                retryMsg = item;
                break;
            }
        }

        // We perform actions further down
        // which may need the sentQ lock, so unlock it here.
        // If another thread enters, all the variables are local
        // the globals are protected so we should still be ok
        sentQLock.unlock();

        // SECTION: Process retry
        if (foundRetry) {

            fpi::SvcUuid svcUuid;
            svcUuid.svc_uuid = retryMsg.first & ~DOMAINRESTART_MASK;

            fpi::SvcInfo svcInfo;

            if (getSvcMgr()->getSvcInfo(svcUuid, svcInfo)) {
                if ( svcInfo.svc_status == fpi::SVC_STATUS_INACTIVE_STOPPED ||
                     svcInfo.svc_status == fpi::SVC_STATUS_INACTIVE_FAILED ) {

                    LOGWARN <<"PM:" << std::hex << svcUuid.svc_uuid << std::dec
                             << " appears to be unreachable, will not retry services"
                             << " start request";
                    removeFromSentQ(retryMsg);

                } else {
                    // Go ahead and retry the message
                    if (retryMsg.first != 0) {

                        LOGNORMAL << "Message to PM:"
                                 << std::hex << svcUuid.svc_uuid << std::dec
                                 << " has not received response for "
                                 << timeElapsed << "seconds. Will re-send";

                        // Erase from sentQueue
                        removeFromSentQ(retryMsg);

                        // Add message to the toSendQueue
                        addToSendQ(retryMsg, true);
                    } else {
                        LOGERROR <<"Evaluated retry msg has bad PM UUID!";
                    }
                }
            } else {
                LOGDEBUG << "Failed to find PM:" << std::hex << svcUuid.svc_uuid
                         << std::dec << "in svcLayer map, will erase from retryQ";

                removeFromSentQ(retryMsg);
            }
        } // end of ifFoundRetry

        std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
    } // end of while
}

void OrchMgr::constructMsgParams(int64_t uuid, NodeUuid& node_uuid, bool& flag) {

    if ( (uuid & DOMAINRESTART_MASK) == DOMAINRESTART_MASK ) {
        flag = true;
    } else {
        flag = false;
    }

    // Clear out the restart flag
    uuid &= ~DOMAINRESTART_MASK;

    node_uuid.uuid_set_val(uuid);

    LOGDEBUG <<"Constructed msgParams, uuid:" << std::hex << uuid << std::dec
             << " , domainRestart:" << flag;
}

void OrchMgr::addToSendQ(PmMsg msg, bool retry)
{
    {
        std::lock_guard<std::mutex> toSendQLock(toSendQMutex);
        // Clear out the last bit for use in the log
        // keep the actual mask(if bit is on) in the msg as is
        int64_t uuid = msg.first;
        uuid &= ~DOMAINRESTART_MASK;

        if (retry) {
            retryMap[uuid] += 1;

            if (retryMap[uuid] > 3) {
                LOGWARN << "Exceeded retry threshold for message to PM:"
                        << std::hex << uuid << std::dec
                        << " , will not re-send start message";
                return;
            }
        }

        LOGDEBUG << "Adding message for PM:"
                    << std::hex << uuid << std::dec
                    << " to the toSendQ";

        toSendMsgQueue.push_back(msg);
    } // release the mutex

    toSendQCondition.notify_one();
}

void OrchMgr::addToSentQ(PmMsg msg) {
    {
        std::lock_guard<std::mutex> sentQLock(sentQMutex);

        int64_t uuid = msg.first & ~DOMAINRESTART_MASK;
        LOGDEBUG << "Adding msg for PM:"
                 << std::hex << uuid << std::dec
                 << " to the sentQ";

        sentMsgQueue.push_back(msg);

    } // release mutex before notifying

    sentQCondition.notify_one();
}

bool OrchMgr::isInSentQ(int64_t uuid) {

    uuid &= ~DOMAINRESTART_MASK;

    bool present = false;
    std::vector<PmMsg>::iterator iter;
    iter = std::find_if (sentMsgQueue.begin(), sentMsgQueue.end(),
                        [uuid](PmMsg msg)->bool
                        {
                        msg.first &= ~DOMAINRESTART_MASK;
                        return uuid == msg.first;
                        });

    if (iter != sentMsgQueue.end()) {
        present = true;
    }

    return present;

}
void OrchMgr::removeFromSentQ(PmMsg sentMsg)
{
    std::lock_guard<std::mutex> sentQLock(sentQMutex);

    int64_t uuid = sentMsg.first & ~DOMAINRESTART_MASK;
    LOGDEBUG << "Removing message to PM:"
             << std::hex << uuid << std::dec
             << " from the sentQ";

    std::vector<PmMsg>::iterator iter;
    iter = std::find_if (sentMsgQueue.begin(), sentMsgQueue.end(),
                        [uuid](PmMsg msg)->bool
                        {
                        msg.first &= ~DOMAINRESTART_MASK;
                        return uuid == msg.first;
                        });

    if (iter != sentMsgQueue.end()) {
        sentMsgQueue.erase(iter);
    } else {
        LOGWARN << "Failed to remove msg to PM:"
                << std::hex << sentMsg.first << std::dec
                << " from sentQ, either msg has already been removed"
                << " or this PM is not well-known";
    }
}
void OrchMgr::start_cfgpath_server()
{
}

void OrchMgr::interrupt_cb(int signum)
{
    LOGDEBUG << "OrchMgr: Shutting down communicator";

    omcp_session_tbl.reset();
    exit(0);
}

VolPolicyMgr *
OrchMgr::om_policy_mgr()
{
    return orchMgr->policy_mgr;
}

const std::string &
OrchMgr::om_stor_prefix()
{
    return orchMgr->stor_prefix;
}

void OrchMgr::defaultS3BucketPolicy()
{
    Error err(ERR_OK);

    FDS_ProtocolInterface::FDSP_PolicyInfoType policy_info;
    policy_info.policy_name = std::string("FDS Default/Stock Policy");
    policy_info.policy_id = 50;
    policy_info.iops_assured = conf_helper_.get<fds_int64_t>("default_iops_min");
    policy_info.iops_throttle = conf_helper_.get<fds_int64_t>("default_iops_max");
    policy_info.rel_prio = 1;

    orchMgr->om_mutex->lock();
    err = orchMgr->policy_mgr->createPolicy(policy_info);
    orchMgr->om_mutex->unlock();
    
    if ( err.ok() )
    {
        LOGDEBUG << "Created  default S3 policy ";
    }
}

bool OrchMgr::loadFromConfigDB() {
    LOGDEBUG << "loading data from ConfigDB ...";

    // check connection
    if (!getConfigDB()->isConnected()) {
        LOGCRITICAL << "unable to talk to ConfigDB ";
        return false;
    }

    // get global domain info
    std::string globalDomain = getConfigDB()->getGlobalDomain();
    if (globalDomain.empty()) {
        /**
         * We assume here that the ConfigDB is being newly constructed and so, set the version.
         */
        getConfigDB()->setConfigVersion();

        std::string version = getConfigDB()->getConfigVersion();
        if (version.empty()) {
            LOGCRITICAL << "Unable to set the initial ConfigDB version. See log for details.";
            return false;
        } else {
            LOGNOTIFY << "Initial creation of ConfigDB at version <" << version << ">.";
        }

        LOGNOTIFY << "Global domain not configured. Setting a default, [fds]";
        getConfigDB()->setGlobalDomain("fds");

    } else {
        /**
         * Existing ConfigDB. We'll take the opportunity to upgrade it if necessary.
         */
        std::string version = getConfigDB()->getConfigVersion();
        if (version.empty()) {
            LOGCRITICAL << "Unable to obtain the ConfigDB version. See log for details.";
            return false;
        } else if (!getConfigDB()->isLatestConfigDBVersion(version)) {
            /**
             * Need to upgrade ConfigDB from current version to latest.
             */
            if (getConfigDB()->upgradeConfigDBVersionLatest(version) != kvstore::ConfigDB::ReturnType::SUCCESS) {
                LOGCRITICAL << "Unable to upgrade the ConfigDB version. See log for details.";
                return false;
            }

        }
    }

    // get local domains
    std::vector<LocalDomain> localDomains;
    getConfigDB()->listLocalDomains(localDomains);

    if (localDomains.empty())  {
        LOGWARN << "No Local Domains stored in the system. "
                << "Setting a default Local Domain.";
        LocalDomain localDomain;

        /**
         * Set this one as the current local domain.
         */
        localDomain.setCurrent(true);
        auto id = getConfigDB()->putLocalDomain(localDomain);
        if (id <= 0) {
            LOGERROR << "Some issue in Local Domain creation. ";
            return false;
        } else {
            LOGNOTIFY << "Default Local Domain creation succeeded. ID: " << id;
        }

        getConfigDB()->listLocalDomains(localDomains);
    }

    if (localDomains.empty()) {
        LOGCRITICAL << "Something wrong with the ConfigDB. "
                    << " -- not loading data.";
        return false;
    }

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();

    // Load current Local Domain.
    for (auto localDomain : localDomains) {
        if (localDomain.getCurrent()) {
            local->setLocalDomain(localDomain);
        }
    }

    if (local->getLocalDomain() == nullptr) {
        LOGCRITICAL << "Unable to set current local domain.";
        return false;
    } else {
        LOGNOTIFY << "Current Local Domain is <" << local->getLocalDomain()->getName()
                  << ":" << local->getLocalDomain()->getID() << ">.";
        if (local->getLocalDomain()->isMaster()) {
            LOGNOTIFY << "Current Local Domain is the Master Local Domain for the Global Domain.";
        }
    }

    // load/create system volumes
    VolumeContainer::pointer volContainer = local->om_vol_mgr();
    volContainer->createSystemVolume();
    volContainer->createSystemVolume(0);

    // keep the pointer in data placement module
    DataPlacement *dp = OM_Module::om_singleton()->om_dataplace_mod();
    dp->setConfigDB(getConfigDB());

    OM_Module::om_singleton()->om_volplace_mod()->setConfigDB(getConfigDB());

    // load the snapshot policies
    if (!test_mode && enableTimeline) {
        snapshotMgr->loadFromConfigDB();
    }

    return true;
}

DmtColumnPtr OrchMgr::getDMTNodesForVolume(fds_volid_t volId) {
    DMTPtr dmt = OM_Module::om_singleton()->om_volplace_mod()->getCommittedDMT();
    return dmt->getNodeGroup(volId);
}

kvstore::ConfigDB* OrchMgr::getConfigDB() {
    return configDB;
}

OrchMgr *gl_orch_mgr;
}  // namespace fds

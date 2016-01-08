/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <arpa/inet.h>

#include <fdsp/common_types.h>
#include <fdsp/ConfigurationService.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/server/TThreadPoolServer.h>

#include <fds_typedefs.h>
#include <fdsp_utils.h>
#include <string>
#include <vector>
#include <util/Log.h>
#include <OmResources.h>
#include <convert.h>
#include <orchMgr.h>
#include <omutils.h>
#include <util/stringutils.h>
#include <util/timeutils.h>
#include <net/PlatNetSvcHandler.h>
#include <kvstore/configdb.h>  // For ConfigDB::ReturnType.


using namespace ::apache::thrift;  //NOLINT
using namespace ::apache::thrift::protocol;  //NOLINT
using namespace ::apache::thrift::transport;  //NOLINT
using namespace ::apache::thrift::server;  //NOLINT
using namespace ::apache::thrift::concurrency;  //NOLINT

namespace fds { namespace  apis {

static void add_service_to_vector(std::vector<fpi::FDSP_Node_Info_Type> &vec,  // NOLINT
                           NodeAgent::pointer ptr) {

    fpi::SvcInfo svcInfo;
    fpi::SvcUuid svcUuid;
    svcUuid.svc_uuid = ptr->rs_get_uuid().uuid_get_val();
    /* Getting from svc map.  Should be able to get it from config db as well */
    if (!MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, svcInfo)) {
        GLOGWARN << "could not find svcinfo for uuid:" << svcUuid.svc_uuid;
        return;
    }
    fpi::FDSP_Node_Info_Type nodeInfo = fpi::FDSP_Node_Info_Type();
    nodeInfo.node_uuid = SvcMgr::mapToSvcUuid(svcUuid, fpi::FDSP_PLATFORM).svc_uuid;
    nodeInfo.service_uuid = svcUuid.svc_uuid;
    nodeInfo.node_name = svcInfo.name;
    nodeInfo.node_type = svcInfo.svc_type;
    nodeInfo.node_state = ptr->node_state();
    nodeInfo.ip_lo_addr =  net::ipString2Addr(svcInfo.ip);
    nodeInfo.control_port = 0;
    nodeInfo.data_port = svcInfo.svc_port;
    nodeInfo.migration_port = 0;
    vec.push_back(nodeInfo);
}

static void add_vol_to_vector(std::vector<FDS_ProtocolInterface::FDSP_VolumeDescType> &vec,  // NOLINT
                       VolumeInfo::pointer vol) {
    FDS_ProtocolInterface::FDSP_VolumeDescType voldesc;
    vol->vol_fmt_desc_pkt(&voldesc);
    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
        << "Volume in list: " << voldesc.vol_name << ":"
        << std::hex << voldesc.volUUID << std::dec
        << "min iops (assured) " << voldesc.iops_assured << ",max iops (throttle)"
        << voldesc.iops_throttle << ", prio " << voldesc.rel_prio;
    vec.push_back(voldesc);
}

class ConfigurationServiceHandler : virtual public ConfigurationServiceIf {
    OrchMgr* om;
    kvstore::ConfigDB* configDB;

  public:
    explicit ConfigurationServiceHandler(OrchMgr* om) : om(om) {
        configDB = om->getConfigDB();
    }

    void apiException(std::string message, fpi::ErrorCode code = fpi::INTERNAL_SERVER_ERROR) {
        LOGERROR << "exception: " << message;
        fpi::ApiException e;
        e.message = message;
        e.errorCode    = code;
        throw e;
    }

    void checkDomainStatus() {
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        if (!domain->om_local_domain_up()) {
            apiException("local domain not up", fpi::SERVICE_NOT_READY);
        }
    }

    /**
     * Use this to detmerine whether we are the Master Domain as some commands
     * may only be executed in the Master Domain.
     */
    void checkMasterDomain() {
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        if (!domain->om_master_domain()) {
            LOGERROR << "Exception: Local Domain not Master.";
            fpi::NotMasterDomain e;
            e.message = "Local Domain not Master.";
            throw e;
        }
    }

    // stubs to keep cpp compiler happy - BEGIN
    int64_t createLocalDomain(const std::string& domainName, const std::string& domainSite) { return 0;}
    void listLocalDomains(std::vector<LocalDomainDescriptor> & _return, const int32_t ignore) {}
    void listLocalDomainsV07(std::vector<LocalDomainDescriptorV07> & _return, const int32_t ignore) {}
    void updateLocalDomainName(const std::string& oldDomainName, const std::string& newDomainName) {}
    void updateLocalDomainSite(const std::string& domainName, const std::string& newSiteName) {}
    void setThrottle(const std::string& domainName, const double throttleLevel) {}
    void setScavenger(const std::string& domainName, const std::string& scavengerAction) {}
    void startupLocalDomain(const std::string& domainName) {}
    int32_t shutdownLocalDomain(const std::string& domainName) { return 0; }
    void deleteLocalDomain(const std::string& domainName) {}
    void activateLocalDomainServices(const std::string& domainName, const bool sm, const bool dm, const bool am) {}
    int32_t ActivateNode(const FDSP_ActivateOneNodeType& act_node_msg) { return 0;}
    int32_t AddService(const fpi::NotifyAddServiceMsg& add_svc_msg) { return 0;}
    int32_t StartService(const fpi::NotifyStartServiceMsg& start_svc_msg) { return 0; }
    int32_t StopService(const fpi::NotifyStopServiceMsg& stop_svc_msg) { return 0; }
    int32_t RemoveService(const fpi::NotifyRemoveServiceMsg& rm_svc_msg) { return 0; }
    void listLocalDomainServices(std::vector<fpi::FDSP_Node_Info_Type>& _return, const std::string& domainName) {}
    void ListServices(std::vector<fpi::FDSP_Node_Info_Type>& ret, const int32_t ignore) {}
    void removeLocalDomainServices(const std::string& domainName, const bool sm, const bool dm, const bool am) {}
    int32_t RemoveServices(const FDSP_RemoveServicesType& rm_svc_req) { return 0; }
    int64_t createTenant(const std::string& identifier) { return 0;}
    void listTenants(std::vector<Tenant> & _return, const int32_t ignore) {}
    int64_t createUser(const std::string& identifier, const std::string& passwordHash, const std::string& secret, const bool isFdsAdmin) { return 0;} //NOLINT
    void assignUserToTenant(const int64_t userId, const int64_t tenantId) {}
    void revokeUserFromTenant(const int64_t userId, const int64_t tenantId) {}
    void allUsers(std::vector<User>& _return, const int64_t ignore) {}
    void listUsersForTenant(std::vector<User> & _return, const int64_t tenantId) {}
    void updateUser(const int64_t userId, const std::string& identifier, const std::string& passwordHash, const std::string& secret, const bool isFdsAdmin) {} //NOLINT
    int64_t configurationVersion(const int64_t ignore) { return 0;}
    void createVolume(const std::string& domainName, const std::string& volumeName, const VolumeSettings& volumeSettings, const int64_t tenantId) {}  //NOLINT
    int64_t getVolumeId(const std::string& volumeName) {return 0;}
    void getVolumeName(std::string& _return, const int64_t volumeId) {}
    void GetVolInfo(fpi::FDSP_VolumeDescType& _return, const FDSP_GetVolInfoReqType& vol_info_req) {}
    int32_t ModifyVol(const FDSP_ModifyVolType& mod_vol_req) {return 0;}
    void deleteVolume(const std::string& domainName, const std::string& volumeName) {}  //NOLINT
    void statVolume(VolumeDescriptor& _return, const std::string& domainName, const std::string& volumeName) {}  //NOLINT
    void listVolumes(std::vector<VolumeDescriptor> & _return, const std::string& domainName) {}  //NOLINT
    void ListVolumes(std::vector<fpi::FDSP_VolumeDescType> & _return, const int32_t ignore) {}
    int32_t registerStream(const std::string& url, const std::string& http_method, const std::vector<std::string> & volume_names, const int32_t sample_freq_seconds, const int32_t duration_seconds) { return 0;} //NOLINT
    void getStreamRegistrations(std::vector<apis::StreamingRegistrationMsg> & _return, const int32_t ignore) {} //NOLINT
    void deregisterStream(const int32_t registration_id) {}
    int64_t createSnapshotPolicy(const  fds::apis::SnapshotPolicy& policy) {return 0;} //NOLINT
    void listSnapshotPolicies(std::vector< fds::apis::SnapshotPolicy> & _return, const int64_t unused) {} //NOLINT
    void deleteSnapshotPolicy(const int64_t id) {} //NOLINT
    void attachSnapshotPolicy(const int64_t volumeId, const int64_t policyId) {} //NOLINT
    void listSnapshotPoliciesForVolume(std::vector< fds::apis::SnapshotPolicy> & _return, const int64_t volumeId) {} //NOLINT
    void detachSnapshotPolicy(const int64_t volumeId, const int64_t policyId) {} //NOLINT
    void listVolumesForSnapshotPolicy(std::vector<int64_t> & _return, const int64_t policyId) {} //NOLINT
    void listSnapshots(std::vector< ::FDS_ProtocolInterface::Snapshot> & _return, const int64_t volumeId) {} //NOLINT
    void createQoSPolicy(fpi::FDSP_PolicyInfoType& _return, const std::string& policyName, const int64_t minIops, const int64_t maxIops, const int32_t relPrio) {}
    void listQoSPolicies(std::vector<fpi::FDSP_PolicyInfoType>& _return, const int64_t unused) {}
    void modifyQoSPolicy(fpi::FDSP_PolicyInfoType& _return, const std::string& current_policy_name, const std::string& new_policy_name, const int64_t iops_min, const int64_t iops_max, const int32_t rel_prio) {};
    void deleteQoSPolicy(const std::string& policyName) {}
    void restoreClone(const int64_t volumeId, const int64_t snapshotId) {} //NOLINT
    int64_t cloneVolume(const int64_t volumeId, const int64_t fdsp_PolicyInfoId, const std::string& clonedVolumeName, const int64_t timelineTime) { return 0;} //NOLINT
    void createSnapshot(const int64_t volumeId, const std::string& snapshotName, const int64_t retentionTime, const int64_t timelineTime) {} //NOLINT
    void deleteSnapshot(const int64_t volumeId, const int64_t snapshotId) {}
    int64_t createSubscription(const std::string& subName, const int64_t tenantID, const int32_t primaryDomainID, const int64_t primaryVolumeID, const int32_t replicaDomainID, const  ::fds::apis::SubscriptionType subType, const  ::fds::apis::SubscriptionScheduleType schedType, const int64_t intervalSize) {return 0;};
    void listSubscriptionsAll(std::vector< ::fds::apis::SubscriptionDescriptor> & _return, const int32_t ignore) {};
    void listTenantSubscriptionsAll(std::vector< ::fds::apis::SubscriptionDescriptor> & _return, const int64_t tenantID) {};
    void listSubscriptionsPrimaryDomain(std::vector< ::fds::apis::SubscriptionDescriptor> & _return, const int32_t primaryDomainID) {};
    void listTenantSubscriptionsPrimaryDomain(std::vector< ::fds::apis::SubscriptionDescriptor> & _return, const int32_t primaryDomainID, const int64_t tenantID) {};
    void listSubscriptionsReplicaDomain(std::vector< ::fds::apis::SubscriptionDescriptor> & _return, const int32_t replicaDomainID) {};
    void listTenantSubscriptionsReplicaDomain(std::vector< ::fds::apis::SubscriptionDescriptor> & _return, const int32_t replicaDomainID, const int64_t tenantID) {};
    void listSubscriptionsPrimaryVolume(std::vector< ::fds::apis::SubscriptionDescriptor> & _return, const int64_t primaryVolumeID) {};
    void listSubscriptionsReplicaVolume(std::vector< ::fds::apis::SubscriptionDescriptor> & _return, const int64_t replicaVolumeID) {};
    void getSubscriptionInfoName( ::fds::apis::SubscriptionDescriptor& _return, const std::string& subName, const int64_t tenantID) {};
    void getSubscriptionInfoID( ::fds::apis::SubscriptionDescriptor& _return, const int64_t subID) {};
    void updateSubscription(const  ::fds::apis::SubscriptionDescriptor& subMods) {};
    void deleteSubscriptionName(const std::string& subName, const int64_t tenantID, const bool dematerialize) {};
    void deleteSubscriptionID(const int64_t subID, const bool dematerialize) {};

    virtual void getNodeInfo( ::FDS_ProtocolInterface::SvcInfo& _return, const  ::FDS_ProtocolInterface::SvcUuid& nodeUuid) {};
    virtual int64_t getDiskCapacityNode(const  ::FDS_ProtocolInterface::SvcUuid& nodeUuid) { return 0; };
    // stubs to keep cpp compiler happy - END

    /**
    * Create a Local Domain.
    *
    * @param domainName - Name of the new Local Domain. Must be unique within Global Domain.
    * @param domainSite - Name of the new Local Domain's site.
    *
    * @return int64_t - ID of the newly created Local Domain.
    */
    int64_t createLocalDomain(boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& domainSite) {
        checkMasterDomain();

        LocalDomain localDomain(*domainName, *domainSite);

        auto id = configDB->putLocalDomain(localDomain);
        if (id <= 0) {
            LOGERROR << "Some issue in Local Domain creation: " << *domainName;
            apiException("Error creating Local Domain.");
        } else {
            LOGNOTIFY << "Local Domain creation succeded. " << id << ": " << *domainName;
        }

        return id;
    }

    /**
    * List the currently defined Local Domains.
    *
    * @param _return - Output vecotor of current Local Domains.
    *
    * @return void.
    */
    void listLocalDomains(std::vector<LocalDomainDescriptor>& _return, boost::shared_ptr<int32_t>& ignore) {
        std::vector<LocalDomain> localDomains;

        checkMasterDomain();

        auto ret = om->getConfigDB()->getLocalDomains(localDomains);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            LOGERROR << "Some issue with retrieving all local domains for the global domain.";
            apiException("Error retrieving all local domains for the global domain.");
        } else {
            _return.clear();

            for (std::size_t i = 0; i < localDomains.size(); i++) {
                apis::LocalDomainDescriptor localDomainDescriptor;
                LocalDomain::makeLocalDomainDescriptor(localDomainDescriptor, localDomains[i]);
                _return.push_back(localDomainDescriptor);
            }
        }
    }

/**
 * Interface version V07.
 *
 * List the currently defined Local Domains.
 *
 * @param _return - Output vecotor of current Local Domains.
 *
 * @return void.
 */
void listLocalDomainsV07(std::vector<LocalDomainDescriptorV07>& _return, boost::shared_ptr<int32_t>& ignore) {
    std::vector<LocalDomain> localDomains;

    checkMasterDomain();

    auto ret = om->getConfigDB()->getLocalDomains(localDomains);

    if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
        LOGERROR << "Some issue with retrieving all local domains for the global domain.";
        apiException("Error retrieving all local domains for the global domain.");
    } else {
        _return.clear();

        for (std::size_t i = 0; i < localDomains.size(); i++) {
            apis::LocalDomainDescriptorV07 localDomainDescriptorV07;

            localDomainDescriptorV07.id = localDomains[i].getID();
            localDomainDescriptorV07.name = localDomains[i].getName();
            localDomainDescriptorV07.site = localDomains[i].getSite();

            _return.push_back(localDomainDescriptorV07);
        }
    }
}
    /**
    * Rename the given Local Domain.
    *
    * @param oldDomainName - Current name of the Local Domain.
    * @param newDomainName - New name of the Local Domain.
    *
    * @return void.
    */
    void updateLocalDomainName(boost::shared_ptr<std::string>& oldDomainName, boost::shared_ptr<std::string>& newDomainName) {
        LocalDomain localDomain;
        checkMasterDomain();

        auto ret = configDB->getLocalDomain(*oldDomainName, localDomain);
        if (ret !=  kvstore::ConfigDB::ReturnType::SUCCESS) {
            LOGERROR << "Some issue in retrieving Local Domain : " << *oldDomainName;
            apiException("Error retrieving Local Domain.");
        }

        localDomain.setName(*newDomainName);

        ret = configDB->updateLocalDomain(localDomain);

        if (ret !=  kvstore::ConfigDB::ReturnType::SUCCESS) {
            LOGERROR << "Some issue in Local Domain rename: " << *oldDomainName;
            apiException("Error renaming Local Domain.");
        } else {
            LOGNOTIFY << "Local Domain <" << localDomain.getID() << "> renamed from <" << *oldDomainName <<
                            "> to <" << *newDomainName << ">.";
        }
    }

    /**
    * Rename the given Local Domain's site.
    *
    * @param domainName - Name of the Local Domain whose site is to be changed.
    * @param newSiteName - New name of the Local Domain's site.
    *
    * @return void.
    */
    void updateLocalDomainSite(boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>&newSite) {
        LocalDomain localDomain;
        checkMasterDomain();

        auto ret = configDB->getLocalDomain(*domainName, localDomain);
        if (ret !=  kvstore::ConfigDB::ReturnType::SUCCESS) {
            LOGERROR << "Some issue in retrieving Local Domain : " << *domainName;
            apiException("Error retrieving Local Domain.");
        }

        auto oldSite = localDomain.getSite();
        localDomain.setSite(*newSite);

        ret = configDB->updateLocalDomain(localDomain);

        if (ret !=  kvstore::ConfigDB::ReturnType::SUCCESS) {
            LOGERROR << "Some issue in Local Domain site update: " << *domainName;
            apiException("Error updating Local Domain site.");
        } else {
            LOGNOTIFY << "Local Domain <" << localDomain.getID() << "> site changed from <" << oldSite <<
                      "> to <" << *newSite << ">.";
        }
    }

    /**
    * Set the throttle level of the given Local Domain.
    *
    * @param domainName - Name of the Local Domain whose throttle level is to be set.
    * @param throtleLevel - New throttel level for the Local Domain.
    *
    * @return void.
    */
    void setThrottle(boost::shared_ptr<std::string>& domainName, boost::shared_ptr<double>& throttleLevel) {
        try {
            assert((*throttleLevel >= -10) &&
                    (*throttleLevel <= 10));

            /*
             * Currently (3/21/2015) we only have support for one Local Domain. So the specified name is ignored.
             * At some point we should be able to look up the DomainContainer based on Domain ID (or name).
             */
            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
            local->om_set_throttle_lvl(static_cast<float>(*throttleLevel));
        }
        catch(...) {
            LOGERROR << "Orch Mgr encountered exception while "
                            << "processing set throttle level";
            apiException("setThrottle caused exception.");
        }
    }

    /**
    * Set the given Local Domain's scavenger action.
    *
    * @param domainName - Name of the Local Domain whose scavenger action is to be set.
    * @param scavengerAction - New scavenger action for the Local Domain. One of "enable", "disable", "start", "stop".
    *
    * @return void.
    */
    void setScavenger(boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& scavengerAction) {
        fpi::FDSP_ScavengerCmd cmd;

        if (*scavengerAction == "enable") {
            cmd = FDS_ProtocolInterface::FDSP_SCAVENGER_ENABLE;
            LOGNOTIFY << "Received scavenger enable command";
        } else if (*scavengerAction == "disable") {
            cmd = FDS_ProtocolInterface::FDSP_SCAVENGER_DISABLE;
            LOGNOTIFY << "Received scavenger disable command";
        } else if (*scavengerAction == "start") {
            cmd =FDS_ProtocolInterface::FDSP_SCAVENGER_START;
            LOGNOTIFY << "Received scavenger start command";
        } else if (*scavengerAction == "stop") {
            cmd =FDS_ProtocolInterface::FDSP_SCAVENGER_STOP;
            LOGNOTIFY << "Received scavenger stop command";
        } else {
            apiException("Unrecognized scavenger action: " + *scavengerAction);
            return;
        };

        /*
         * Currently (3/21/2015) we only have support for one Local Domain. So the specified name is ignored.
         * At some point we should be able to look up the DomainContainer based on Domain ID (or name).
         */
        // send scavenger start message to all SMs
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        local->om_bcast_scavenger_cmd(cmd);
    }

    void startupLocalDomain(boost::shared_ptr<std::string>& domainName)
    {
        LocalDomain localDomain;
        checkMasterDomain();

        auto ret = configDB->getLocalDomain(*domainName, localDomain);
        if ( ret != kvstore::ConfigDB::ReturnType::SUCCESS )
        {
            LOGERROR << "Local Domain not found: " << domainName;

            apiException( "Error starting Local Domain " +
                          *domainName +
                          ". Local Domain not found." );
        }

        /*
         * Currently (05/05/2015) we only have support for one Local Domain.
         * So the specified name is ignored. At some point we should be able
         * to look up the DomainContainer based on Domain ID (or name).
         */

        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        try
        {
            domain->om_startup_domain();
        }
        catch(...)
        {
            LOGERROR << "Orch Manager encountered exception while "
                            << "processing startup local domain";
            apiException( "Error starting up Local Domain " +
                          *domainName +
                          " Services. Broadcast startup failed." );
        }
    }

    /**
    * Shutdown the named Local Domain.
    *
    * @return void.
    */
    int32_t shutdownLocalDomain(boost::shared_ptr<std::string>& domainName) {
        LocalDomain localDomain;
        checkMasterDomain();
        Error err(ERR_OK);

        auto ret = configDB->getLocalDomain(*domainName, localDomain);
        if (ret !=  kvstore::ConfigDB::ReturnType::SUCCESS) {
            LOGERROR << "Some issue in retrieving Local Domain : " << *domainName;
            apiException("Error retrieving Local Domain.");
        }

        /*
         * Currently (3/21/2015) we only have support for one Local Domain.
         * So the specified name is ignored. At some point we should be able
         * to look up the DomainContainer based on Domain ID (or name).
         */

        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();

        try {
            LOGNORMAL << "Received shutdown for Local Domain " << domainName;

            err = domain->om_shutdown_domain();
        }
        catch(...) {
            LOGERROR << "Orch Mgr encountered exception while "
                            << "processing shutdown local domain";
            apiException("Error shutting down Local Domain " + *domainName + " Services. Broadcast shutdown failed.");
        }

        return err.GetErrno();
    }

    /**
    * Delete the Local Domain.
    *
    * @param domainName - Name of the Local Domain to be deleted.
    *
    * @return void.
    */
    void deleteLocalDomain(boost::shared_ptr<std::string>& domainName) {
        checkMasterDomain();

        auto domainID = configDB->getLocalDomainId(*domainName);

        if (domainID <= 0) {
            LOGERROR << "Local Domain not found: " << domainName;
            apiException("Error deleting Local Domain " + *domainName + ". Local Domain not found.");
        }

        configDB->deleteLocalDomain(domainID);
    }

    /**
    * Activate all defined Services for all Nodes defined for the named Local Domain.
    *
    * If all Service flags are set to False, it will
    * be interpreted to mean activate all Services currently defined for the Node. If there are
    * no Services currently defined for the node, it will be interpreted to mean activate all
    * Services on the Node (SM, DM, and AM), and define all Services for the Node.
    *
    * @param domainName - Name of the Local Domain whose services are to be activated.
    * @param sm - An fds_bool_t indicating whether the SM Service should be activated (True) or not (False)
    * @param dm - An fds_bool_t indicating whether the DM Service should be activated (True) or not (False)
    * @param am - An fds_bool_t indicating whether the AM Service should be activated (True) or not (False)
    *
    * @return void.
    */
    void activateLocalDomainServices(boost::shared_ptr<std::string>& domainName,
                                     boost::shared_ptr<fds_bool_t>& sm,
                                     boost::shared_ptr<fds_bool_t>& dm,
                                     boost::shared_ptr<fds_bool_t>& am) {
        auto domainID = configDB->getLocalDomainId(*domainName);

        if (domainID <= 0) {
            LOGERROR << "Local Domain not found: " << *domainName;
            apiException("Error activating Local Domain " + *domainName + " Services. Local Domain not found.");
        }

        /*
         * Currently (3/21/2015) we only have support for one Local Domain. So the specified name is ignored.
         * At some point we should be able to look up the DomainContainer based on Domain ID (or name).
         */

        OM_NodeContainer *localDomain = OM_NodeDomainMod::om_loc_domain_ctrl();

        try {
            LOGNORMAL << "Received activate services for Local Domain " << *domainName;
            LOGNORMAL << "SM: " << *sm << "; DM: " << *dm << "; AM: " << *am;

            localDomain->om_cond_bcast_start_services(*sm, *dm, *am);
        }
        catch(...) {
            LOGERROR << "Orch Mgr encountered exception while "
                            << "processing activate all node services";
            apiException("Error activating Local Domain " + *domainName + " Services. Broadcast activate services failed.");
        }

    }

    int32_t ActivateNode(boost::shared_ptr<FDSP_ActivateOneNodeType>& act_node_msg) {
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
                                                   act_node_msg->activate_dm,
                                                   act_node_msg->activate_am);
        }
        catch(...) {
            LOGERROR << "Orch Mgr encountered exception while "
                     << "processing activate all nodes";
            err = Error(ERR_NOT_FOUND);  // need some better error code
        }

        return err.GetErrno();
    }

    int32_t AddService(boost::shared_ptr<::FDS_ProtocolInterface::NotifyAddServiceMsg>& add_svc_msg) {
        Error err(ERR_OK);
        fpi::SvcUuid pmUuid;
        try {
            LOGNORMAL << "Received Add Service request";
            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
            std::vector<fpi::SvcInfo> svcInfos = add_svc_msg->services;

            for (fpi::SvcInfo svcInfo : svcInfos) {
                if ( svcInfo.svc_type == fpi::FDSP_PLATFORM ) {
                    pmUuid = svcInfo.svc_id.svc_uuid;
                }
            }

            err = local->om_add_service(pmUuid, svcInfos);
        }
        catch(...) {
            LOGERROR <<"Orch Mgr encountered exception while "
                     << "processing add service";
            err = Error(ERR_NOT_FOUND);
        }
        return err.GetErrno();
    }

    int32_t StartService(boost::shared_ptr<::FDS_ProtocolInterface::NotifyStartServiceMsg>& start_svc_msg) {
        Error err(ERR_OK);
        fpi::SvcUuid pmUuid;
        try {
            LOGNORMAL << "Received Start Service request";
            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
            std::vector<fpi::SvcInfo> svcInfos = start_svc_msg->services;

            bool startNode = start_svc_msg->isActionNodeStart;
            for (fpi::SvcInfo svcInfo : svcInfos) {
                if ( svcInfo.svc_type == fpi::FDSP_PLATFORM ) {
                    pmUuid = svcInfo.svc_id.svc_uuid;
                }
            }

            bool domainRestart = false;
            err = local->om_start_service(pmUuid, svcInfos, domainRestart, startNode);
        }
        catch(...){
            LOGERROR <<"Orch Mgr encountered exception while "
                     << "processing start service";
            err = Error(ERR_NOT_FOUND);
        }
        return err.GetErrno();
    }

    int32_t StopService(boost::shared_ptr<::FDS_ProtocolInterface::NotifyStopServiceMsg>& stop_svc_msg) {
        Error err(ERR_OK);
        fpi::SvcUuid pmUuid;

        try {
            LOGNORMAL << "Received stop service request";

            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
            bool stop_sm = false;
            bool stop_dm = false;
            bool stop_am = false;
            bool shutdownNode = stop_svc_msg->isActionNodeShutdown;
            std::vector<fpi::SvcInfo> svcInfos = stop_svc_msg->services;

            // We need to know which services are being stopped
            // since there are specific actions to be taken against each service
            for (fpi::SvcInfo svcInfo : svcInfos) {
                if ( svcInfo.svc_type == fpi::FDSP_PLATFORM ) {
                    pmUuid = svcInfo.svc_id.svc_uuid;
                }
                else if (svcInfo.svc_type == fpi::FDSP_STOR_MGR)
                {
                    stop_sm = true;
                }
                else if (svcInfo.svc_type == fpi::FDSP_DATA_MGR)
                {
                    stop_dm = true;
                }
                else if (svcInfo.svc_type == fpi::FDSP_ACCESS_MGR)
                {
                    stop_am = true;
                }
            }

            err = local->om_stop_service(pmUuid, svcInfos, stop_sm, stop_dm, stop_am, shutdownNode);
        }
        catch(...) {
            LOGERROR << "Orch Mgr encountered exception while "
                     << "processing stop service";
            err = Error(ERR_NOT_FOUND);
        }

        return err.GetErrno();
    }

    int32_t RemoveService(boost::shared_ptr<::FDS_ProtocolInterface::NotifyRemoveServiceMsg>& rm_svc_msg) {
        Error err(ERR_OK);
        fpi::SvcUuid pmUuid;

        try {
            LOGNORMAL << "Received remove service request";

            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
            bool remove_sm = false;
            bool remove_dm = false;
            bool remove_am = false;
            bool removeNode = rm_svc_msg->isActionNodeRemove;
            std::vector<fpi::SvcInfo> svcInfos = rm_svc_msg->services;

            // We need to know which services are being removed
            // since there are specific actions to be taken against each service
            for (fpi::SvcInfo svcInfo : svcInfos) {
                if ( svcInfo.svc_type == fpi::FDSP_PLATFORM ) {
                    pmUuid = svcInfo.svc_id.svc_uuid;
                }
                else if (svcInfo.svc_type == fpi::FDSP_STOR_MGR)
                {
                    remove_sm = true;
                }
                else if (svcInfo.svc_type == fpi::FDSP_DATA_MGR)
                {
                    remove_dm = true;
                }
                else if (svcInfo.svc_type == fpi::FDSP_ACCESS_MGR)
                {
                    remove_am = true;
                }
            }

            err = local->om_remove_service(pmUuid, svcInfos, remove_sm, remove_dm, remove_am, removeNode);
        }
        catch(...) {
            LOGERROR << "Orch Mgr encountered exception while "
                     << "processing remove service";
            err = Error(ERR_NOT_FOUND);
        }
        return err.GetErrno();
    }

    /**
    * List all defined Services for all Nodes defined for the named Local Domain.
    *
    * @param _return - Output vector of Services.
    * @param domainName - Name of the Local Domain whose services are to be activated.
    *
    * @return void.
    */
    void listLocalDomainServices(std::vector<fpi::FDSP_Node_Info_Type>& _return,
                                 boost::shared_ptr<std::string>& domainName) {
        /*
         * Currently (3/18/2015) only support for one Local Domain.
         * So the specified name is ignored. Also, we should be using Domain UUID.
         */

        std::vector<fpi::SvcInfo> svcinfos;
        if ( configDB->getSvcMap( svcinfos ) )
        {
            if ( svcinfos.size( ) > 0 )
            {
                for ( const fpi::SvcInfo svcinfo : svcinfos )
                {
                    _return.push_back( fds::fromSvcInfo( svcinfo ) );
                }
            }
            else
            {
                LOGNORMAL << "No persisted local domain ( "
                          << domainName
                          << " ) services found.";
            }
        }

        // always return ourself FS-1779: OM does not report status to UI
        _return.push_back( fds::fromSvcInfo( fds::gl_orch_mgr->getSvcMgr()->getSelfSvcInfo() ) );
    }

    /**
     * A alias of listLocalDomainServices() and should be removed.
     */
    void ListServices(std::vector<fpi::FDSP_Node_Info_Type>& vec, boost::shared_ptr<int32_t>& ignore) {
        boost::shared_ptr<std::string> ldomain = boost::make_shared<std::string>("local");
        listLocalDomainServices(vec, ldomain);
        return;
    }

    /**
    * Remove all defined Services from all Nodes defined for the named Local Domain.
    *
    * If all Service flags are set to False, it will
    * be interpreted to mean remove all Services currently defined for the Node.
    * Removal means that the Service is unregistered from the Domain and shutdown.
    *
    * @param domainName - Name of the Local Domain whose services are to be removed.
    * @param sm - An fds_bool_t indicating whether the SM Service should be removed (True) or not (False)
    * @param dm - An fds_bool_t indicating whether the DM Service should be removed (True) or not (False)
    * @param am - An fds_bool_t indicating whether the AM Service should be removed (True) or not (False)
    *
    * @return void.
    */
    void removeLocalDomainServices(boost::shared_ptr<std::string>& domainName,
                                   boost::shared_ptr<fds_bool_t>& sm,
                                   boost::shared_ptr<fds_bool_t>& dm,
                                   boost::shared_ptr<fds_bool_t>& am) {
        auto domainID = configDB->getLocalDomainId(*domainName);

        if (domainID <= 0) {
            LOGERROR << "Local Domain not found: " << domainName;
            apiException("Error removing Local Domain " + *domainName + " Services. Local Domain not found.");
        }

        /*
         * Currently (3/21/2015) we only have support for one Local Domain. So the specified name is ignored.
         * At some point we should be able to look up the DomainContainer based on Domain ID (or name).
         */

        OM_NodeContainer *localDomain = OM_NodeDomainMod::om_loc_domain_ctrl();

        try {
            LOGNORMAL << "Received remove services for Local Domain " << domainName;
            LOGNORMAL << "SM: " << *sm << "; DM: " << *dm << "; AM: " << *am;

            localDomain->om_cond_bcast_remove_services(*sm, *dm, *am);
        }
        catch(...) {
            LOGERROR << "Orch Mgr encountered exception while "
                            << "processing remove all node services";
            apiException("Error removing Local Domain " + *domainName + " Services. Broadcast remove services failed.");
        }

    }


    int32_t RemoveServices(boost::shared_ptr<FDSP_RemoveServicesType>& rm_svc_req) {

        Error err(ERR_OK);
        try {
            LOGNORMAL << "Received remove services for node" << rm_svc_req->node_name
                      << " UUID " << std::hex << rm_svc_req->node_uuid.uuid << std::dec
                      << " remove am ? " << rm_svc_req->remove_am
                      << " remove sm ? " << rm_svc_req->remove_sm
                      << " remove dm ? " << rm_svc_req->remove_dm;

            OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
            NodeUuid node_uuid;
            if (rm_svc_req->node_uuid.uuid > 0) {
                node_uuid = rm_svc_req->node_uuid.uuid;
            }
            // else ok if 0, will search by name

            err = domain->om_del_services(rm_svc_req->node_uuid.uuid,
                                          rm_svc_req->node_name,
                                          rm_svc_req->remove_sm,
                                          rm_svc_req->remove_dm,
                                          rm_svc_req->remove_am);

            if (!err.ok()) {
                LOGERROR << "RemoveServices: Failed to remove services for node "
                         << rm_svc_req->node_name << ", uuid "
                         << std::hex << rm_svc_req->node_uuid.uuid
                         << std::dec << ", result: " << err.GetErrstr();
            }
        }
        catch(...) {
            LOGERROR << "Orch Mgr encountered exception while "
                     << "processing rmv node";
            err = Error(ERR_NOT_FOUND);
        }

        return err.GetErrno();
    }

    int64_t createTenant(boost::shared_ptr<std::string>& identifier) {
        int64_t tenantId =  configDB->createTenant(*identifier);

        // create the system volume associated to the client
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();

        volContainer->createSystemVolume(tenantId);
        return tenantId;
    }

    void listTenants(std::vector<Tenant> & _return, boost::shared_ptr<int32_t>& ignore) {
        configDB->listTenants(_return);
    }

    int64_t createUser(boost::shared_ptr<std::string>& identifier,
                       boost::shared_ptr<std::string>& passwordHash,
                       boost::shared_ptr<std::string>& secret,
                       boost::shared_ptr<bool>& isFdsAdmin) {
        return configDB->createUser(*identifier, *passwordHash, *secret, *isFdsAdmin);
    }

    void assignUserToTenant(boost::shared_ptr<int64_t>& userId,
                            boost::shared_ptr<int64_t>& tenantId) {
        configDB->assignUserToTenant(*userId, *tenantId);
    }

    void revokeUserFromTenant(boost::shared_ptr<int64_t>& userId,
                              boost::shared_ptr<int64_t>& tenantId) {
        configDB->revokeUserFromTenant(*userId, *tenantId);
    }

    void allUsers(std::vector<User> & _return, boost::shared_ptr<int64_t>& ignore) {
        configDB->listUsers(_return);
    }

    void listUsersForTenant(std::vector<User> & _return, boost::shared_ptr<int64_t>& tenantId) {
        configDB->listUsersForTenant(_return, *tenantId);
    }

    void updateUser(boost::shared_ptr<int64_t>& userId,
                    boost::shared_ptr<std::string>& identifier,
                    boost::shared_ptr<std::string>& passwordHash,
                    boost::shared_ptr<std::string>& secret,
                    boost::shared_ptr<bool>& isFdsAdmin) {
        configDB->updateUser(*userId, *identifier, *passwordHash, *secret, *isFdsAdmin);
    }

    int64_t configurationVersion(boost::shared_ptr<int64_t>& ignore) {
        int64_t ver =  configDB->getLastModTimeStamp();
        // LOGDEBUG << "config version : " << ver;
        return ver;
    }

    void createVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<VolumeSettings>& volumeSettings,
                      boost::shared_ptr<int64_t>& tenantId) {
        LOGNOTIFY << " domain: " << *domainName
                  << " volume: " << *volumeName
                  << " tenant: " << *tenantId;

        checkDomainStatus();

        switch ( volumeSettings->volumeType )
        {
            case apis::BLOCK:
                break;
            case apis::ISCSI:
                LOGDEBUG << "iSCSI:: "
                         << "LUN count [ " << volumeSettings->iscsiTarget.luns.size() << " ] "
                         << "Initiator count [ " << volumeSettings->iscsiTarget.initiators.size() << " ] "
                         << "Incoming Users count [ " << volumeSettings->iscsiTarget.incomingUsers.size() << " ] "
                         << "Outgoing Users count [ " << volumeSettings->iscsiTarget.outgoingUsers.size() << " ]";
                break;
            case apis::NFS:
                LOGDEBUG << "NFS:: [ " << volumeSettings->nfsOptions.client << " ] "
                         << "[ " << volumeSettings->nfsOptions.options << " ]";
                break;
            case apis::OBJECT:
                break;
        }

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        VolumeInfo::pointer vol = volContainer->get_volume(*volumeName);
        Error err = volContainer->getVolumeStatus(*volumeName);
        if (err == ERR_OK)
        {
            apiException( "Volume ( " + *volumeName + " ) already exists", fpi::RESOURCE_ALREADY_EXISTS);
        }

        fpi::FDSP_MsgHdrTypePtr header;
        FDSP_CreateVolTypePtr request;

        convert::getFDSPCreateVolRequest(header, request,
                                         *domainName, *volumeName, *volumeSettings);
        request->vol_info.tennantId = *tenantId;
        err = volContainer->om_create_vol(header, request);
        if ( err != ERR_OK )
        {
            apiException( "Error creating volume ( " + *volumeName + " ) - " + err.GetErrstr() );
        }

        // wait for the volume to be active upto 5 minutes
        int count = 600;

        do {
            usleep(500000);  // 0.5s
            vol = volContainer->get_volume(*volumeName);
            count--;
        } while (count > 0 && vol && !vol->isStateActive());

        if (!vol || !vol->isStateActive())
        {
            std::string emsg = "Error creating volume ( " + *volumeName + " ) - Timeout waiting for volume to become ACTIVE";
            LOGERROR << emsg;
            apiException( emsg );
        }
    }

    int64_t getVolumeId(boost::shared_ptr<std::string>& volumeName) {
        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        VolumeInfo::pointer  vol = VolumeInfo::vol_cast_ptr(volContainer->rs_get_resource(volumeName->c_str())); //NOLINT
        if (vol) {
            return vol->rs_get_uuid().uuid_get_val();
        } else {
            LOGWARN << "The specified volume " << *volumeName << " was not found.";
            return 0;
        }
    }

    void getVolumeName(std::string& volumeName, boost::shared_ptr<int64_t>& volumeId) {
        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        VolumeInfo::pointer  vol = VolumeInfo::vol_cast_ptr(volContainer->rs_get_resource(*volumeId)); //NOLINT
        if (vol) {
            volumeName =  vol->vol_get_name();
        } else {
            LOGWARN << "Unable to get volume info for vol:" << *volumeId;
        }
    }

    void GetVolInfo(fpi::FDSP_VolumeDescType& _return, boost::shared_ptr<FDSP_GetVolInfoReqType>& vol_info_req) {
        LOGDEBUG << "Received Get volume info request for volume: "
                 << vol_info_req->vol_name;

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer vol_list = local->om_vol_mgr();
        VolumeInfo::pointer vol = vol_list->get_volume(vol_info_req->vol_name);
        if (vol) {
            vol->vol_fmt_desc_pkt(&_return);
            LOGDEBUG << "Volume " << vol_info_req->vol_name
                     << " -- min iops (assured) " << _return.iops_assured
                     << ",max iops (throttle) " << _return.iops_throttle
                     << ", prio " << _return.rel_prio
                     << " media policy " << _return.mediaPolicy;
        } else {
            FDSP_VolumeNotFound except;
            except.message = std::string("Volume " + vol_info_req->vol_name + " not found");
            LOGWARN << except.message;
            throw except;
        }
    }

    int32_t ModifyVol(FDSP_ModifyVolTypePtr& mod_vol_req) {
        Error err(ERR_OK);
        LOGNOTIFY << "Received modify volume [ " << (mod_vol_req->vol_desc).vol_name << " ]"
                  << " [ " << (mod_vol_req->vol_desc).volUUID << " ]"
                  << " [ " << (mod_vol_req->vol_desc).volType << " ]";

        if ( ( mod_vol_req->vol_desc ).volType == fpi::FDSP_VOL_ISCSI_TYPE )
        {
            FDS_ProtocolInterface::IScsiTarget iscsi = ( mod_vol_req->vol_desc ).iscsi;
            LOGDEBUG << "iSCSI:: LUN count [ " << iscsi.luns.size() << " ]"
                     << " Initiator count [ " << iscsi.initiators.size() << " ]"
                     << " Incoming Users count [ " << iscsi.incomingUsers.size() << " ]"
                     << " Outgoing Users count [ " << iscsi.outgoingUsers.size() << " ]";
        }
        else if ( ( mod_vol_req->vol_desc ).volType == fpi::FDSP_VOL_NFS_TYPE )
        {
            FDS_ProtocolInterface::NfsOption nfs = ( mod_vol_req->vol_desc ).nfs;
            LOGDEBUG << "NFS:: [ " << nfs.client << " ] "
                     << "[ " << nfs.options << " ]";
        }

        try {
            // no need to check if local domain is up, because volume create
            // would be rejected so om_modify_vol will return error in that case
            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
            err = local->om_modify_vol(mod_vol_req);
        }
        catch(...) {
            LOGERROR << "Orch Mgr encountered exception while "
                     << "processing modify volume";
            err = Error(ERR_NETWORK_TRANSPORT);  // only transport throws
        }

        return err.GetErrno();
    }

    void deleteVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName) {
        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        Error err = volContainer->getVolumeStatus(*volumeName);

        if (err != ERR_OK) apiException("volume ( " + *volumeName + " ) does NOT exist", fpi::MISSING_RESOURCE);

        fpi::FDSP_MsgHdrTypePtr header;
        apis::FDSP_DeleteVolTypePtr request;
        convert::getFDSPDeleteVolRequest(header, request, *domainName, *volumeName);
        err = volContainer->om_delete_vol(header, request);
        LOGNOTIFY << "delete volume processed for :" << *volumeName << " " << err;
    }

    void statVolume(VolumeDescriptor& volDescriptor,
                    boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName) {
        checkDomainStatus();
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        Error err = volContainer->getVolumeStatus(*volumeName);
        if (err != ERR_OK) apiException( "volume ( " + *volumeName + " ) NOT FOUND" , fpi::MISSING_RESOURCE);

        VolumeInfo::pointer  vol = volContainer->get_volume(*volumeName);

        convert::getVolumeDescriptor(volDescriptor, vol);
    }

    void listVolumes(std::vector<VolumeDescriptor> & _return,
                     boost::shared_ptr<std::string>& domainName) {

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();

        volContainer->vol_up_foreach<std::vector<VolumeDescriptor> &>(_return, [] (std::vector<VolumeDescriptor> &vec, VolumeInfo::pointer vol) { //NOLINT
            LOGDEBUG << (vol->vol_get_properties()->isSnapshot() ? "snapshot" : "volume")
                     << " [ " << vol->vol_get_name() << " ] type [ " << vol->vol_get_properties()->volType
                     << " ] state [ " << vol->vol_get_properties()->getStateName() << " ] ";

            if (!vol->vol_get_properties()->isSnapshot()) {
                if (vol->getState() == fpi::Active) {
                    VolumeDescriptor volDescriptor;
                    convert::getVolumeDescriptor(volDescriptor, vol);
                    vec.push_back(volDescriptor);

                    if ( volDescriptor.policy.volumeType == apis::ISCSI )
                    {
                        FDS_ProtocolInterface::IScsiTarget iscsi = volDescriptor.policy.iscsiTarget;
                        LOGDEBUG << "iSCSI:: LUN count [ " << iscsi.luns.size() << " ]"
                                 << " Initiator count [ " << iscsi.initiators.size() << " ]"
                                 << " Incoming Users count [ " << iscsi.incomingUsers.size() << " ]"
                                 << " Outgoing Users count [ " << iscsi.outgoingUsers.size() << " ]";
                    }
                    else if (volDescriptor.policy.volumeType == apis::NFS )
                    {
                        FDS_ProtocolInterface::NfsOption nfs = volDescriptor.policy.nfsOptions;
                        LOGDEBUG << "NFS:: [ " << nfs.client << " ] "
                                 << "[ " << nfs.options << " ]";
                    }
                }
            }
        });
    }

    void ListVolumes(std::vector<fpi::FDSP_VolumeDescType> & _return, boost::shared_ptr<int32_t>& ignore) {
        LOGDEBUG<< "OM received ListVolumes message";
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer vols = local->om_vol_mgr();
        // list volumes that are not in 'delete pending' state
        vols->vol_up_foreach<std::vector<fpi::FDSP_VolumeDescType> &>(_return, add_vol_to_vector);
    }

    int32_t registerStream(boost::shared_ptr<std::string>& url,
                           boost::shared_ptr<std::string>& http_method,
                           boost::shared_ptr<std::vector<std::string> >& volume_names,
                           boost::shared_ptr<int32_t>& sample_freq_seconds,
                           boost::shared_ptr<int32_t>& duration_seconds) {
        int32_t regId = configDB->getNewStreamRegistrationId();
        apis::StreamingRegistrationMsg regMsg;
        regMsg.id = regId;
        regMsg.url = *url;
        regMsg.http_method = *http_method;
        regMsg.sample_freq_seconds = *sample_freq_seconds;
        regMsg.duration_seconds = *duration_seconds;

        LOGDEBUG << "Received register stream for frequency "
                 << regMsg.sample_freq_seconds << " seconds"
                 << " duration " << regMsg.duration_seconds << " seconds";

        regMsg.volume_names.reserve(volume_names->size());
        for (uint i = 0; i < volume_names->size(); i++) {
            regMsg.volume_names.push_back(volume_names->at(i));
        }

        if (configDB->addStreamRegistration(regMsg)) {
            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
            local->om_bcast_stream_register_cmd(regId, false);
            return regId;
        }
        return 0;
    }

    void getStreamRegistrations(std::vector<apis::StreamingRegistrationMsg> & _return,
                                boost::shared_ptr<int32_t>& ignore) { //NOLINT
        configDB->getStreamRegistrations(_return);
    }

    void deregisterStream(boost::shared_ptr<int32_t>& registration_id) {
        LOGDEBUG << "De-registering stream id " << *registration_id;
        if ( configDB->removeStreamRegistration(*registration_id) )
        {
            LOGDEBUG << "broadcasting De-registration for stream id "
                     << *registration_id;
            /*
             * FS-2561 Tinius 07/16/2015
             * tell everyone to de-register this stat stream.
             */
            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl( );
            local->om_bcast_stream_de_register_cmd( *registration_id );
        }
    }

    int64_t createSnapshotPolicy(boost::shared_ptr<fds::apis::SnapshotPolicy>& policy) {
        /*
         * OK, if we leave this defaulted to 'false' we will never create snapshot policies
         * which I believe isn't what we want. Because all volumes created will not have snapshot
         * policies so if or when this feature is enabled, set to true, these volumes will fail
         * because not snapshot policy exists.
         *
         * We just don't want to add it to the scheduler.
         *
         * P. Tinius 12/23/2015 changed:
         *  if (om->enableTimeline && configDB->createSnapshotPolicy(*policy)) {
         */
        if ( configDB->createSnapshotPolicy( *policy ) )
        {
            LOGDEBUG << "Snapshot Schedule is " << om->enableTimeline;
            if ( om->enableTimeline )
            {
                om->snapshotMgr->addPolicy( *policy );
            }

            return policy->id;
        }
        return -1;
    }

    void listSnapshotPolicies(std::vector<fds::apis::SnapshotPolicy> & _return,
                      boost::shared_ptr<int64_t>& unused) {
        configDB->listSnapshotPolicies(_return);
    }

    void deleteSnapshotPolicy(boost::shared_ptr<int64_t>& id) {
        LOGDEBUG << "Snapshot Schedule is " << om->enableTimeline ? "enabled" : "disabled";
        /*
         * P. Tinius 12/23/2015 changed:
         *  if (om->enableTimeline) {
         *
         *  ADDED: log message below.
         */
        configDB->deleteSnapshotPolicy(*id);
        om->snapshotMgr->removePolicy(*id);
    }

    void attachSnapshotPolicy(boost::shared_ptr<int64_t>& volumeId,
                              boost::shared_ptr<int64_t>& policyId) {
        configDB->attachSnapshotPolicy(fds_volid_t(*volumeId), *policyId);
    }

    void listSnapshotPoliciesForVolume(std::vector<fds::apis::SnapshotPolicy> & _return,
                                       boost::shared_ptr<int64_t>& volumeId) {
        configDB->listSnapshotPoliciesForVolume(_return, fds_volid_t(*volumeId));
    }

    void detachSnapshotPolicy(boost::shared_ptr<int64_t>& volumeId,
                              boost::shared_ptr<int64_t>& policyId) {
        configDB->detachSnapshotPolicy(fds_volid_t(*volumeId), *policyId);
    }

    void listVolumesForSnapshotPolicy(std::vector<int64_t> & _return,
                              boost::shared_ptr<int64_t>& policyId) {
        configDB->listVolumesForSnapshotPolicy(_return, *policyId);
    }

    void listSnapshots(std::vector<fpi::Snapshot> & _return,
                       boost::shared_ptr<int64_t>& volumeId) {
        configDB->listSnapshots(_return, fds_volid_t(*volumeId));
    }

    /**
    * Create a QoS Policy.
    *
    * @param _return - Output create QoS Policy details.
    * @param policyName - Name of the new QoS Policy. Must be unique within Global Domain.
    * @param domainSite - Name of the new Local Domain's site.
    */
    void createQoSPolicy(fpi::FDSP_PolicyInfoType& _return, boost::shared_ptr<std::string>& policyName,
                           boost::shared_ptr<int64_t>& minIops, boost::shared_ptr<int64_t>& maxIops,
                           boost::shared_ptr<int32_t>& relPrio ) {
        LOGNOTIFY << "Received CreatePolicy  Msg for policy "
                  << policyName;

        auto policy_mgr = om->om_policy_mgr();
        policy_mgr->createPolicy(_return, *policyName,
                                 static_cast<fds_uint64_t>(*minIops), static_cast<fds_uint64_t>(*maxIops),
                                 static_cast<fds_uint32_t>(*relPrio));

        if (_return.policy_id > 0) {
            LOGNOTIFY << "QoS Policy creation succeded. " << _return.policy_id << ": " << *policyName;
        } else {
            LOGERROR << "Some issue in QoS Policy creation: " << *policyName;
            apiException("Error creating QoS Policy.");
        }
    }

    /**
    * List the currently defined QoS Policies.
    *
    * @param _return - Output vector of current QoS Policies.
    *
    * @return void.
    */
    void listQoSPolicies(std::vector<fpi::FDSP_PolicyInfoType>& _return, boost::shared_ptr<int64_t>& ignore) {
        std::vector<FDS_VolumePolicy> qosPolicies;

        if (configDB->getPolicies(qosPolicies)) {
            for (const auto& qosPolicy : qosPolicies) {
                fpi::FDSP_PolicyInfoType fdspQoSPolicy;

                fdspQoSPolicy.policy_name = qosPolicy.volPolicyName;
                fdspQoSPolicy.policy_id = qosPolicy.volPolicyId;
                fdspQoSPolicy.iops_assured = qosPolicy.iops_assured;
                fdspQoSPolicy.iops_throttle = qosPolicy.iops_throttle;
                fdspQoSPolicy.rel_prio = qosPolicy.relativePrio;

                _return.push_back(fdspQoSPolicy);
            }
        } else {
            LOGERROR << "Some issue in listing QoS Policies.";
            apiException("Error listing QoS Policies.");
        }
    }

    /**
     * Modify a QoS Policy.
     *
     * @param _return - Output modified QoS Policy details.
     * @param currentPolicyName - Name of the current QoS Policy. Must be unique within Global Domain.
     * @param newPolicyName - Name of the new QoS Policy. Must be unique within Global Domain. May be the same as
     *                        currentPolicyName if the name is not being changed.
     * @param minIops - New policy minimum IOPS.
     * @param maxIops - New policy maximum IOPS.
     * @param relPrio - New policy relative priority.
     */
    void modifyQoSPolicy(fpi::FDSP_PolicyInfoType& _return,
                         boost::shared_ptr<std::string>& currentPolicyName, boost::shared_ptr<std::string>& newPolicyName,
                         boost::shared_ptr<int64_t>& minIops, boost::shared_ptr<int64_t>& maxIops,
                         boost::shared_ptr<int32_t>& relPrio ) {
        fds_assert(*relPrio >= 0);
        FDS_VolumePolicy qosPolicy;

        qosPolicy.volPolicyId = configDB->getIdOfQoSPolicy(*currentPolicyName);

        if (qosPolicy.volPolicyId > 0) {
            qosPolicy.volPolicyName = *newPolicyName;
            qosPolicy.iops_assured = *minIops;
            qosPolicy.iops_throttle = *maxIops;
            qosPolicy.relativePrio = static_cast<fds_uint32_t>(*relPrio);
            if (configDB->updatePolicy(qosPolicy)) {
                _return.policy_id = qosPolicy.volPolicyId;
                _return.policy_name = qosPolicy.volPolicyName;
                _return.iops_assured = qosPolicy.iops_assured;
                _return.iops_throttle = qosPolicy.iops_throttle;
                _return.rel_prio = qosPolicy.relativePrio;

                LOGNOTIFY << "QoS Policy modification succeded. " << _return.policy_id << ": " << *currentPolicyName;
            } else {
                LOGERROR << "Some issue in QoS Policy modification: " << *currentPolicyName;
                apiException("Error modifying QoS Policy.");
            }
        } else {
            LOGNOTIFY << "No policy found for " << *currentPolicyName;
        }

        return;
    }

    /**
    * Delete the QoS Policy.
    *
    * @param policyName - Name of the QoS Policy to be deleted.
    *
    * @return void.
    */
    void deleteQoSPolicy(boost::shared_ptr<std::string>& policyName) {
        auto policyId = configDB->getIdOfQoSPolicy(*policyName);

        if (policyId > 0) {
            if (configDB->deletePolicy(policyId)) {
                LOGNOTIFY << "QoS Policy delete succeded. " << policyId << ": " << *policyName;
            } else {
                LOGERROR << "Some issue in QoS Policy deletion: " << *policyName;
                apiException("Error deleting QoS Policy.");
            }
        } else {
            LOGNOTIFY << "No policy found for " << *policyName;
        }
    }

    void restoreClone(boost::shared_ptr<int64_t>& volumeId,
                         boost::shared_ptr<int64_t>& snapshotId) {
        apiException("restore clone - functionality NOT IMPLEMENTED");
    }

    int64_t cloneVolume(boost::shared_ptr<int64_t>& volumeId,
                        boost::shared_ptr<int64_t>& volPolicyId,
                        boost::shared_ptr<std::string>& clonedVolumeName,
                        boost::shared_ptr<int64_t>& timelineTime) {
        checkDomainStatus();
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        VolPolicyMgr      *volPolicyMgr = om->om_policy_mgr();
        VolumeInfo::pointer  parentVol, vol;
        if (!om->enableTimeline) {
            apiException("attempting to clone volume but feature disabled");
        }

        vol = volContainer->get_volume(*clonedVolumeName);
        if (vol != NULL) {
            LOGWARN << "volume with same name already exists : " << *clonedVolumeName;
            apiException("volume with same name already exists");
        }

        parentVol = VolumeInfo::vol_cast_ptr(volContainer->rs_get_resource(*volumeId));
        if (parentVol == NULL) {
            LOGWARN << "unable to locate source volume info : " << *volumeId;
            apiException("unable to locate source volume info");
        }

        VolumeDesc desc(*(parentVol->vol_get_properties()));

        desc.volUUID = configDB->getNewVolumeId();
        if (invalid_vol_id == desc.volUUID) {
            LOGWARN << "unable to generate a new vol id";
            apiException("unable to generate a new vol id");
        }
        desc.name = *clonedVolumeName;
        if (*volPolicyId > 0) {
            desc.volPolicyId = *volPolicyId;
        }
        desc.backupVolume = invalid_vol_id.get();
        desc.fSnapshot = false;
        desc.srcVolumeId = *volumeId;
        desc.timelineTime = *timelineTime;
        desc.createTime = util::getTimeStampSeconds();

        if (parentVol->vol_get_properties()->lookupVolumeId == invalid_vol_id) {
            desc.lookupVolumeId = *volumeId;
        } else {
            desc.lookupVolumeId = parentVol->vol_get_properties()->lookupVolumeId;
        }

        desc.qosQueueId = invalid_vol_id;
        volPolicyMgr->fillVolumeDescPolicy(&desc);
        LOGDEBUG << "adding a clone request..";
        desc.setState(fpi::ResourceState::Loading);
        volContainer->addVolume(desc);

        // wait for the volume to be active upto 30 seconds
        int count = 60;
        do {
            usleep(500000);  // 0.5s
            vol = volContainer->get_volume(*clonedVolumeName);
            count--;
        } while (count > 0 && vol && !vol->isStateActive());

        if (!vol || !vol->isStateActive()) {
            LOGERROR << "some issue in volume cloning";
            apiException("error creating volume");
        } else {
            // volume created successfully ,
            // now create a base snapshot. [FS-471]
            // we have to do this here because only OM can create a new
            // volume id
            boost::shared_ptr<int64_t> sp_volId(new int64_t(vol->rs_get_uuid().uuid_get_val()));
            boost::shared_ptr<std::string> sp_snapName(new std::string(
                util::strformat("snap0_%s_%d", clonedVolumeName->c_str(),
                                util::getTimeStampSeconds())));
            boost::shared_ptr<int64_t> sp_retentionTime(new int64_t(0));
            boost::shared_ptr<int64_t> sp_timelineTime(new int64_t(0));
            createSnapshot(sp_volId, sp_snapName, sp_retentionTime, sp_timelineTime);
        }

        return vol->vol_get_properties()->volUUID.get();
    }

    void createSnapshot(boost::shared_ptr<int64_t>& volumeId,
                        boost::shared_ptr<std::string>& snapshotName,
                        boost::shared_ptr<int64_t>& retentionTime,
                        boost::shared_ptr<int64_t>& timelineTime) {
                    // create the structure
        if (!om->enableTimeline) {
            apiException("attempting to create snapshot but feature disabled");
        }
        fpi::Snapshot snapshot;
        snapshot.snapshotName = util::strlower(*snapshotName);
        snapshot.volumeId = *volumeId;
        auto snapshotId = configDB->getNewVolumeId();
        if (invalid_vol_id == snapshotId) {
            LOGWARN << "unable to generate a new snapshot id";
            apiException("unable to generate a new snapshot id");
        }
        snapshot.snapshotId = snapshotId.get();
        snapshot.snapshotPolicyId = 0;
        snapshot.creationTimestamp = util::getTimeStampSeconds();
        snapshot.retentionTimeSeconds = *retentionTime;
        snapshot.timelineTime = *timelineTime;

        snapshot.state = fpi::ResourceState::Loading;
        LOGDEBUG << "snapshot request for volume id:" << snapshot.volumeId
                 << " name:" << snapshot.snapshotName;

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        fds::Error err = volContainer->addSnapshot(snapshot);
        if ( !err.ok() ) {
            LOGWARN << "snapshot add failed : " << err;
            apiException(err.GetErrstr());
        }
        om->snapshotMgr->deleteScheduler->addSnapshot(snapshot);
    }

    void deleteSnapshot(boost::shared_ptr<int64_t>& volumeId, boost::shared_ptr<int64_t>& snapshotId) {
        checkDomainStatus();
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        fpi::Snapshot snapshot;
        if (!om->enableTimeline) {
            apiException("attempting to delete snapshot but feature disabled");
        }

        snapshot.volumeId = *volumeId;
        snapshot.snapshotId = *snapshotId;
        if (!om->getConfigDB()->getSnapshot(snapshot)) {
            apiException(util::strformat("snapshot not found for [vol:%ld] - [snap:%ld]",*volumeId, *snapshotId));
        }
        snapshot.state = fpi::ResourceState::MarkedForDeletion;
        // mark the snapshot for deletion
        om->getConfigDB()->updateSnapshot(snapshot);

        volContainer->om_delete_vol(fds_volid_t(snapshot.snapshotId));
    }

    /** Subscription Management **/
    /**
     * Create a subscription.
     *
     * @param name - Name of the new Subscription. Must be unique within Global Domain/Tenant.
     * @param tenantID - ID of the Tenant owning the new Subscription.
     * @param primaryDomainID - ID of the Local Domain where the primary copy of the replicated volume is located.
     * @param primaryVolumeID - ID of the primary volume as defined in the primary Domain.
     * @param replicaDomainID - ID of the Local Domain where the replica copy of the replicated volume is located.
     * @param subType - Flag indicating the type of asynchronous repliction used with this Subscription.
     * @param schedType - For subscription types using a content-based replication technique, this flag indicates
     *                    the type of scheduling (time, MB, num ops, etc.) using for this subscription.
     * @param intervalSize - For subscription types using a content-based replication technique, this value defines
     *                       the number of units for an interval to expire. Units are implied by schedType.
     *
     * @return int64_t - ID of the newly created Subscription.
     */
    int64_t createSubscription(boost::shared_ptr<std::string>& name,
                               boost::shared_ptr<int64_t>& tenantID,
                               boost::shared_ptr<int32_t>& primaryDomainID,
                               boost::shared_ptr<int64_t>& primaryVolumeID,
                               boost::shared_ptr<int32_t>& replicaDomainID,
                               boost::shared_ptr< ::fds::apis::SubscriptionType>& subType,
                               boost::shared_ptr< ::fds::apis::SubscriptionScheduleType>& schedType,
                               boost::shared_ptr<int64_t>& intervalSize) {
        checkMasterDomain();

        Subscription subscription(name.get()->c_str(),
                                  *tenantID,
                                  *primaryDomainID,
                                  fds_volid_t(static_cast<uint64_t>(*primaryVolumeID)),
                                  *replicaDomainID,
                                  apis::SubscriptionType(*subType),
                                  apis::SubscriptionScheduleType(*schedType),
                                  *intervalSize);
        fds_subid_t id = om->getConfigDB()->putSubscription(subscription);

        if (id == invalid_sub_id) {
            LOGERROR << "Some issue in subscription creation: " << *name;
            apiException("Error creating subscription [" + *name + "].");
        } else {
            LOGNOTIFY << "Subscription creation succeded. " << id << ": " << *name;
        }

        return id;
    }

    /**
     * List all subscriptions defined in the global domain.
     *
     * @param _return - An output parameter. A list of all subscriptions defined in the global domain and their
     *                  details. Could be empty.
     * @param ignore - As the name suggests, not used for anything other than to make Thrift work nice.
     *
     * @return Nothing.
     */
    void listSubscriptionsAll(std::vector< ::fds::apis::SubscriptionDescriptor> & _return,
                              boost::shared_ptr<int32_t>& ignore) {
        std::vector<Subscription> subscriptions;

        checkMasterDomain();

        kvstore::ConfigDB::ReturnType ret = om->getConfigDB()->getSubscriptions(subscriptions);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            LOGERROR << "Some issue with retrieving all subscriptions for the global domain.";
            apiException("Error retrieving all subscriptions for the global domain.");
        } else {
            _return.clear();

            for (std::size_t i = 0; i < subscriptions.size(); i++) {
                apis::SubscriptionDescriptor subscription;
                Subscription::makeSubscriptionDescriptor(subscription, subscriptions[i]);
                _return.push_back(subscription);
            }
        }
    };

    /**
     * List all subscriptions defined in the global domain for the given tenant.
     *
     * @param _return - An output parameter. A list of all subscriptions defined in the global domain and tenant and their
     *                  details. Could be empty.
     * @param tenantID - ID of the tenant for whom subscriptions are to be retrieved.
     *
     * @return Nothing.
     */
    void listTenantSubscriptionsAll(std::vector< ::fds::apis::SubscriptionDescriptor> & _return,
                                    boost::shared_ptr<int64_t>& tenantID) {
        std::vector<Subscription> subscriptions;

        checkMasterDomain();

        kvstore::ConfigDB::ReturnType ret = om->getConfigDB()->getSubscriptions(subscriptions);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            std::string tenantIDStr = std::to_string(*tenantID);
            LOGERROR << "Some issue with retrieving all subscriptions for tenant [" << tenantIDStr << "].";
            apiException("Error retrieving all subscriptions for tenant [" + tenantIDStr + "].");
        } else {
            _return.clear();

            for (std::size_t i = 0; i < subscriptions.size(); i++) {
                if (subscriptions[i].getTenantID() == *tenantID) {
                    apis::SubscriptionDescriptor subscription;
                    Subscription::makeSubscriptionDescriptor(subscription, subscriptions[i]);
                    _return.push_back(subscription);
                }
            }
        }
    };

    /**
     * List all subscriptions defined with the given local domain as primary.
     *
     * @param _return - An output parameter. A list of all subscriptions defined with the given local domain as primary
     *                  and their details. Could be empty.
     * @param primaryDomainID - ID of the local domain identified as "primary" for the subscriptions to be retrieved.
     *
     * @return Nothing.
     */
    void listSubscriptionsPrimaryDomain(std::vector< ::fds::apis::SubscriptionDescriptor> & _return,
                                        boost::shared_ptr<int32_t>& primaryDomainID) {
        std::vector<Subscription> subscriptions;

        checkMasterDomain();

        kvstore::ConfigDB::ReturnType ret = om->getConfigDB()->getSubscriptions(subscriptions);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            std::string primaryDomainIDStr = std::to_string(*primaryDomainID);
            LOGERROR << "Some issue with retrieving all subscriptions for the primary domain [" << primaryDomainIDStr << "].";
            apiException("Error retrieving all subscriptions for the primary domain [" + primaryDomainIDStr + "].");
        } else {
            _return.clear();

            for (std::size_t i = 0; i < subscriptions.size(); i++) {
                if (subscriptions[i].getPrimaryDomainID() == *primaryDomainID) {
                    apis::SubscriptionDescriptor subscription;
                    Subscription::makeSubscriptionDescriptor(subscription, subscriptions[i]);
                    _return.push_back(subscription);
                }
            }
        }
    };

    /**
     * List all subscriptions defined with the given local domain as primary and given tenant.
     *
     * @param _return - An output parameter. A list of all subscriptions defined with the given local domain as primary
     *                  and given tenant and their details. Could be empty.
     * @param primaryDomainID - ID of the local domain identified as "primary" for the subscriptions to be retrieved.
     * @param tenantID - ID of the tenant for whom subscriptions are to be retrieved.
     *
     * @return Nothing.
     */
    void listTenantSubscriptionsPrimaryDomain(std::vector< ::fds::apis::SubscriptionDescriptor> & _return,
                                              boost::shared_ptr<int32_t>& primaryDomainID,
                                              boost::shared_ptr<int64_t>& tenantID) {
        std::vector<Subscription> subscriptions;

        checkMasterDomain();

        kvstore::ConfigDB::ReturnType ret = om->getConfigDB()->getSubscriptions(subscriptions);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            std::string primaryDomainIDStr = std::to_string(*primaryDomainID);
            std::string tenantIDStr = std::to_string(*tenantID);
            LOGERROR << "Some issue with retrieving all subscriptions for the primary domain [" << primaryDomainIDStr <<
                            "] and tenant [" << tenantIDStr << "].";
            apiException("Error retrieving all subscriptions for the primary domain [" + primaryDomainIDStr +
                                 "] and tenant [" + tenantIDStr + "].");
        } else {
            _return.clear();

            for (std::size_t i = 0; i < subscriptions.size(); i++) {
                if ((subscriptions[i].getPrimaryDomainID() == *primaryDomainID) &&
                    (subscriptions[i].getTenantID() == *tenantID)) {
                    apis::SubscriptionDescriptor subscription;
                    Subscription::makeSubscriptionDescriptor(subscription, subscriptions[i]);
                    _return.push_back(subscription);
                }
            }
        }
    };

    /**
     * List all subscriptions defined with the given local domain as replica.
     *
     * @param _return - An output parameter. A list of all subscriptions defined with the given local domain as replica
     *                  and their details. Could be empty.
     * @param replicaDomainID - ID of the local domain identified as "replica" for the subscriptions to be retrieved.
     *
     * @return Nothing.
     */
    void listSubscriptionsReplicaDomain(std::vector< ::fds::apis::SubscriptionDescriptor> & _return,
                                        boost::shared_ptr<int32_t>& replicaDomainID) {
        std::vector<Subscription> subscriptions;

        checkMasterDomain();

        kvstore::ConfigDB::ReturnType ret = om->getConfigDB()->getSubscriptions(subscriptions);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            std::string replicaDomainIDStr = std::to_string(*replicaDomainID);
            LOGERROR << "Some issue with retrieving all subscriptions for the replica domain [" << replicaDomainIDStr << "].";
            apiException("Error retrieving all subscriptions for the primary domain [" + replicaDomainIDStr + "].");
        } else {
            _return.clear();

            for (std::size_t i = 0; i < subscriptions.size(); i++) {
                if (subscriptions[i].getPrimaryDomainID() == *replicaDomainID) {
                    apis::SubscriptionDescriptor subscription;
                    Subscription::makeSubscriptionDescriptor(subscription, subscriptions[i]);
                    _return.push_back(subscription);
                }
            }
        }
    };

    /**
     * List all subscriptions defined with the given local domain as replica and given tenant.
     *
     * @param _return - An output parameter. A list of all subscriptions defined with the given local domain as replica
     *                  and given tenant and their details. Could be empty.
     * @param replicaDomainID - ID of the local domain identified as "replica" for the subscriptions to be retrieved.
     * @param tenantID - ID of the tenant for whom subscriptions are to be retrieved.
     *
     * @return Nothing.
     */
    void listTenantSubscriptionsReplicaDomain(std::vector< ::fds::apis::SubscriptionDescriptor> & _return,
                                              boost::shared_ptr<int32_t>& replicaDomainID,
                                              boost::shared_ptr<int64_t>& tenantID) {
        std::vector<Subscription> subscriptions;

        checkMasterDomain();

        kvstore::ConfigDB::ReturnType ret = om->getConfigDB()->getSubscriptions(subscriptions);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            std::string replicaDomainIDStr = std::to_string(*replicaDomainID);
            std::string tenantIDStr = std::to_string(*tenantID);
            LOGERROR << "Some issue with retrieving all subscriptions for the replica domain [" << replicaDomainIDStr <<
                     "] and tenant [" << tenantIDStr << "].";
            apiException("Error retrieving all subscriptions for the replica domain [" + replicaDomainIDStr +
                         "] and tenant [" + tenantIDStr + "].");
        } else {
            _return.clear();

            for (std::size_t i = 0; i < subscriptions.size(); i++) {
                if ((subscriptions[i].getPrimaryDomainID() == *replicaDomainID) &&
                    (subscriptions[i].getTenantID() == *tenantID)) {
                    apis::SubscriptionDescriptor subscription;
                    Subscription::makeSubscriptionDescriptor(subscription, subscriptions[i]);
                    _return.push_back(subscription);
                }
            }
        }
    };

    /**
     * List all subscriptions defined with the given volume as primary. Since a volume may be owned by only one
     * tenant, tenant is implied.
     *
     * @param _return - An output parameter. A list of all subscriptions defined with the given volume as primary and their
     *                  details. Could be empty.
     * @param primaryVolumeID - ID of the volume identified as "primary" for the subscriptions to be retrieved.
     *
     * @return Nothing.
     */
    void listSubscriptionsPrimaryVolume(std::vector< ::fds::apis::SubscriptionDescriptor> & _return,
                                        boost::shared_ptr<int64_t>& primaryVolumeID) {
        std::vector<Subscription> subscriptions;

        checkMasterDomain();

        kvstore::ConfigDB::ReturnType ret = om->getConfigDB()->getSubscriptions(subscriptions);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            std::string primaryVolumeIDStr = std::to_string(*primaryVolumeID);
            LOGERROR << "Some issue with retrieving all subscriptions for the primary volume [" << primaryVolumeIDStr << "].";
            apiException("Error retrieving all subscriptions for the primary volume [" + primaryVolumeIDStr + "].");
        } else {
            _return.clear();

            for (std::size_t i = 0; i < subscriptions.size(); i++) {
                if (subscriptions[i].getPrimaryVolumeID() == fds_volid_t(static_cast<uint64_t>(*primaryVolumeID))) {
                    apis::SubscriptionDescriptor subscription;
                    Subscription::makeSubscriptionDescriptor(subscription, subscriptions[i]);
                    _return.push_back(subscription);
                }
            }
        }
    };

    /**
     * List all subscriptions defined with the given volume as replica. Since a volume may be owned by only one
     * tenant, tenant is implied.
     *
     * @param _return - An output parameter. A list of all subscriptions defined with the given volume as replica and their
     *                  details. Could be empty.
     * @param replicaVolumeID - ID of the volume identified as "replica" for the subscriptions to be retrieved.
     *
     * @return Nothing.
     */
    void listSubscriptionsReplicaVolume(std::vector< ::fds::apis::SubscriptionDescriptor> & _return,
                                        boost::shared_ptr<int64_t>& replicaVolumeID) {
        std::vector<Subscription> subscriptions;

        checkMasterDomain();

        kvstore::ConfigDB::ReturnType ret = om->getConfigDB()->getSubscriptions(subscriptions);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            std::string replicaVolumeIDStr = std::to_string(*replicaVolumeID);
            LOGERROR << "Some issue with retrieving all subscriptions for the replica volume [" << replicaVolumeIDStr << "].";
            apiException("Error retrieving all subscriptions for the replica volume [" + replicaVolumeIDStr + "].");
        } else {
            _return.clear();

            /**
             * TODO(Greg): Implement mechanism to obtain replica volume ID from subscription.
            for (std::size_t i = 0; i < subscriptions.size(); i++) {
                if (subscriptions[i].getReplicaVolumeID() == fds_volid_t(static_cast<uint64_t>(*replicaVolumeID))) {
                    apis::SubscriptionDescriptor subscription;
                    Subscription::makeSubscriptionDescriptor(subscription, subscriptions[i]);
                    _return.push_back(subscription);
                }
            }
             */
            LOGWARN << "Listing subscirptions for a given replica volume not currently implemented.";
        }
    };

    /**
     * Retrieve all the details for the subscription identified by name and tenant ID. Note that it is not sufficient
     * when referencing a subscription by name to not also provide the tenant ID since multiple tenants could have
     * liked-named subsciptions.
     *
     * @param _return - An output parameter. The detail of the subscription identified by name and tenant ID.
     * @param subName - Name of the subscription for whom details are to be retrieved.
     * @param tenantID - ID of the tenant for whom the named subscription is to be retrieved.
     *
     * @return Nothing.
     */
    void getSubscriptionInfoName( ::fds::apis::SubscriptionDescriptor& _return,
                                  boost::shared_ptr<std::string>& subName,
                                  boost::shared_ptr<int64_t>& tenantID) {
        Subscription subscription;

        checkMasterDomain();

        kvstore::ConfigDB::ReturnType ret = om->getConfigDB()->getSubscription(*subName, *tenantID, subscription);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            std::string tenantIDStr = std::to_string(*tenantID);
            LOGERROR << "Some issue with retrieving subscription named [" << *subName << "] for tenant [" << tenantIDStr << "].";
            apiException("Error retrieving subscription named [" + *subName + "] for tenant [" + tenantIDStr + "].");
        } else if (ret == kvstore::ConfigDB::ReturnType::NOT_FOUND) {
            std::string tenantIDStr = std::to_string(*tenantID);
            LOGERROR << "Unable to locate subscription named [" << *subName << "] for tenant [" << tenantIDStr << "].";

            apis::SubscriptionNotFound e;
            e.message = "Unable to locate subscription named [" + *subName + "] for tenant [" + tenantIDStr + "].";
            throw e;
        } else {
            fds_assert(ret == kvstore::ConfigDB::ReturnType::SUCCESS);
            Subscription::makeSubscriptionDescriptor(_return, subscription);
        }
    };

    /**
     * Retrieve all the details for the subscription identified by unique ID.
     *
     * @param _return - An output parameter. The detail of the subscription identified by name and tenant ID.
     * @param subID - ID of the subscription for whom details are to be retrieved.
     *
     * @return Nothing.
     */
    void getSubscriptionInfoID( ::fds::apis::SubscriptionDescriptor& _return,
                                boost::shared_ptr<int64_t>& subID) {
        Subscription subscription;

        checkMasterDomain();

        kvstore::ConfigDB::ReturnType ret = om->getConfigDB()->getSubscription(*subID, subscription);

        if (ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION) {
            std::string subIDStr = std::to_string(*subID);
            LOGERROR << "Some issue with retrieving subscription by ID [" << subIDStr << "].";
            apiException("Error retrieving subscription by ID [" + subIDStr + "].");
        } else if (ret == kvstore::ConfigDB::ReturnType::NOT_FOUND) {
            std::string subIDStr = std::to_string(*subID);
            LOGERROR << "Unable to locate subscription ID [" << subIDStr << "].";

            apis::SubscriptionNotFound e;
            e.message = "Unable to locate subscription ID [" + subIDStr + "].";
            throw e;
        } else {
            fds_assert(ret == kvstore::ConfigDB::ReturnType::SUCCESS);
            Subscription::makeSubscriptionDescriptor(_return, subscription);
        }
    };

    /**
     * Update subscription attributes. The following are *not* modifiable:
     * id, tenantID, primaryDomainID, primaryVolumeID, replicaDomainID, createTime
     *
     * @param subMods - All subscription attributes with modifications applied. "id" must be supplied. Changes to the
     *                  following are rejected: tenantID, primaryDomainID, primaryVolumeID, replicaDomainID, createTime
     *
     * @return void.
     */
    void updateSubscription(boost::shared_ptr< ::fds::apis::SubscriptionDescriptor>& subMods) {
        checkMasterDomain();

        Subscription subscription("dummy", invalid_sub_id);

        Subscription::makeSubscription(subscription, *subMods);
        std::string tenantIDStr = std::to_string(subscription.getTenantID());

        auto ret = om->getConfigDB()->updateSubscription(subscription);

        if (ret == kvstore::ConfigDB::ReturnType::SUCCESS) {
            LOGNOTIFY << "Subscription update succeded. " << tenantIDStr << ": " << subscription.getName();
        } else if (ret == kvstore::ConfigDB::ReturnType::NOT_FOUND) {
            LOGERROR << "Unable to locate subscription named [" << subscription.getName() <<
                            "] for tenant [" << tenantIDStr << "].";

            apis::SubscriptionNotFound e;
            e.message = "Unable to locate subscription named [" + subscription.getName() +
                        "] for tenant [" + tenantIDStr + "].";
            throw e;
        } else if (ret == kvstore::ConfigDB::ReturnType::NOT_UPDATED) {
            LOGERROR << "Unable to apply updates to subscription named [" << subscription.getName() <<
                     "] for tenant [" << tenantIDStr << "].";

            apis::SubscriptionNotModified e;
            e.message = "Unable to apply updates to subscription named [" + subscription.getName() +
                        "] for tenant [" + tenantIDStr + "].";
            throw e;
        } else {
            fds_assert(ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION);
            LOGERROR << "Some issue with updating subscription named [" << subscription.getName() <<
                     "] for tenant [" << tenantIDStr << "].";
            apiException("Error updating subscription named [" + subscription.getName() +
                         "] for tenant [" + tenantIDStr + "].");
        }
    };

    /**
     * Delete the subscription identified by name and tenant ID. Optionally remove from the replica, those blobs maintained
     * by the subscription. Note that it is not sufficient, when referencing a subscription by name to not also provide
     * the tenant ID since multiple tenants could have liked-named subsciptions.
     *
     * @param subName - Name of the subscription to be deleted.
     * @param tenantID - ID of the tenant for whom the named subscription is to be deleted.
     * @param dematerialize - "true" if the replica volume contents resulting from the subscription are to be
     *                        deleted. If setting "true" results in all replica volume contents being deleted, the
     *                        volume will be deleted as well.
     *
     * @return Nothing.
     */
    void deleteSubscriptionName(boost::shared_ptr<std::string>& subName,
                                boost::shared_ptr<int64_t>& tenantID,
                                boost::shared_ptr<bool>& dematerialize) {
        checkMasterDomain();

        std::string tenantIDStr = std::to_string(*tenantID);

        auto subID = om->getConfigDB()->getSubscriptionId(*subName, *tenantID);

        if (subID == invalid_sub_id) {
            LOGERROR << "Some issue with retrieving subscription ID for [" << *subName << "] and tenant [" << tenantIDStr << "].";
            apiException("Error retrieving subscription ID for [" + *subName + "] and tenant [" + tenantIDStr + "].");
        } else {
            auto ret = om->getConfigDB()->deleteSubscription(subID);

            if (ret == kvstore::ConfigDB::ReturnType::SUCCESS) {
                LOGNOTIFY << "Subscription delete succeded for subscription named [" << *subName <<
                             "] and tenant [" << tenantIDStr << "].";
            } else if (ret == kvstore::ConfigDB::ReturnType::NOT_FOUND) {
                LOGERROR << "Unable to locate subscription named [" << *subName <<
                         "] for tenant [" << tenantIDStr << "].";

                apis::SubscriptionNotFound e;
                e.message = "Unable to locate subscription named [" + *subName +
                            "] for tenant [" + tenantIDStr + "].";
                throw e;
            } else if (ret == kvstore::ConfigDB::ReturnType::NOT_UPDATED) {
                LOGERROR << "Unable to delete subscription named [" << *subName <<
                         "] for tenant [" << tenantIDStr << "].";

                apis::SubscriptionNotModified e;
                e.message = "Unable to delete subscription named [" + *subName +
                            "] for tenant [" + tenantIDStr + "].";
                throw e;
            } else {
                fds_assert(ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION);
                LOGERROR << "Some issue with deleting subscription named [" << *subName <<
                         "] for tenant [" << tenantIDStr << "].";
                apiException("Error deleting subscription named [" + *subName +
                             "] for tenant [" + tenantIDStr + "].");
            }
        }

        if (dematerialize) {
            // TODO(Greg)
        }
    };

    /**
     * Delete the subscription identified by unique ID.
     *
     * @param subID - ID of the subscription to be deleted.
     * @param dematerialize - "true" if the replica volume contents resulting from the subscription are to be
     *                        deleted. If setting "true" results in all replica volume contents being deleted, the
     *                        volume will be deleted as well.
     *
     * @return Nothing.
     */
    void deleteSubscriptionID(boost::shared_ptr<int64_t>& subID,
                              boost::shared_ptr<bool>& dematerialize) {
        checkMasterDomain();

        std::string subIDStr = std::to_string(*subID);

        auto ret = om->getConfigDB()->deleteSubscription(*subID);

        if (ret == kvstore::ConfigDB::ReturnType::SUCCESS) {
            LOGNOTIFY << "Subscription delete succeded for subscription ID [" << subIDStr << "].";
        } else if (ret == kvstore::ConfigDB::ReturnType::NOT_FOUND) {
            LOGERROR << "Unable to locate subscription ID [" << subIDStr << "].";

            apis::SubscriptionNotFound e;
            e.message = "Unable to locate subscription ID [" + subIDStr + "].";
            throw e;
        } else if (ret == kvstore::ConfigDB::ReturnType::NOT_UPDATED) {
            LOGERROR << "Unable to delete subscription ID [" << subIDStr << "].";

            apis::SubscriptionNotModified e;
            e.message = "Unable to delete subscription ID [" + subIDStr + "].";
            throw e;
        } else {
            fds_assert(ret == kvstore::ConfigDB::ReturnType::CONFIGDB_EXCEPTION);
            LOGERROR << "Some issue with deleting subscription ID [" << subIDStr << "].";
            apiException("Error deleting subscription ID [" + subIDStr + "].");
        }

        if (dematerialize) {
            // TODO(Greg)
        }
    };

    void getAllNodeInfo(std::vector< ::FDS_ProtocolInterface::SvcInfo> & _return) {

        std::vector<fpi::SvcInfo> svcInfos;
        bool success = om->getConfigDB()->getSvcMap( svcInfos );
        if (success && svcInfos.size() > 0) {

            for (fpi::SvcInfo svcInfo : svcInfos) {
                if ( svcInfo.svc_type == fpi::FDSP_PLATFORM ) {
                    _return.push_back(svcInfo);
                }
            }
        } else {
            LOGERROR << "Failed to retrieve Service Map";
            apiException("Failed to retrieve Service Map");
        }
    }

    void getNodeInfo( ::FDS_ProtocolInterface::SvcInfo& _return,
                      boost::shared_ptr< ::FDS_ProtocolInterface::SvcUuid>& nodeUuid) {
        std::vector<fpi::SvcInfo> svcInfos;
        getAllNodeInfo(svcInfos);
        for (fpi::SvcInfo svcInfo : svcInfos) {
            if ( svcInfo.svc_id.svc_uuid == *nodeUuid ) {
                _return = svcInfo;
                return;
            }
        }

        apiException("Failed to retrieve service info for node[" + getNodeUuidString(nodeUuid.get()) + "]");
    }

    int64_t getDiskCapacityNode(boost::shared_ptr< ::FDS_ProtocolInterface::SvcUuid>& nodeUuid) {
        fpi::SvcInfo *svcInfo = nullptr;
        getNodeInfo(*svcInfo, nodeUuid);

        if (svcInfo == nullptr) {
            apiException("Failed to retrieve service info for node[" + getNodeUuidString(nodeUuid.get()) + "]");
        }

        return std::stol(svcInfo->props.at("disk_capacity")) + std::stol(svcInfo->props.at("ssd_capacity"));

    }

    std::string getNodeUuidString(fpi::SvcUuid* nodeUuid) {
        std::stringstream nodeUuidStr;
        nodeUuidStr << std::hex << nodeUuid->svc_uuid << std::dec;
        return nodeUuidStr.str();
    }

    int64_t getDiskCapacityTotal() {
        int64_t total = 0;
        std::vector<fpi::SvcInfo> svcInfos;
        bool success = om->getConfigDB()->getSvcMap( svcInfos );
        if (success && svcInfos.size() > 0) {

            for (fpi::SvcInfo svcInfo : svcInfos) {
                if ( svcInfo.svc_type == fpi::FDSP_PLATFORM ) {
                    total += std::stol(svcInfo.props.at("disk_capacity"));
                    total += std::stol(svcInfo.props.at("ssd_capacity"));
                }
            }

            return total;
        } else {
            LOGERROR << "Failed to retrieve Service Map";
            apiException("Failed to retrieve Service Map");
        }
        return -1;
    };

};
}  // namespace apis

std::thread* runConfigService(OrchMgr* om) {
    int port = MODULEPROVIDER()->get_conf_helper().get_abs<int>("fds.om.config_port", 9090);
    LOGDEBUG << "Starting Configuration Service, listening on port: " << port;

    boost::shared_ptr<TServerTransport> serverTransport(
        new TServerSocket( port ) );  //NOLINT
    boost::shared_ptr<TTransportFactory> transportFactory(
        new TFramedTransportFactory( ) );
    boost::shared_ptr<TProtocolFactory> protocolFactory(
        new TBinaryProtocolFactory( ) );  //NOLINT

    boost::shared_ptr<apis::ConfigurationServiceHandler> handler(
        new apis::ConfigurationServiceHandler( om ) ); // NOLINT
    boost::shared_ptr<TProcessor> processor(
        new apis::ConfigurationServiceProcessor( handler ) ); // NOLINT

    TThreadedServer server( processor,
                            serverTransport,
                            transportFactory,
                            protocolFactory );

    server.serve();

    return nullptr;
}

}  // namespace fds

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <arpa/inet.h>

#include <apis/snapshot_types.h>
#include <apis/ConfigurationService.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include <NetSession.h>
#include <fds_typedefs.h>
#include <string>
#include <vector>
#include <util/Log.h>
#include <OmResources.h>
#include <convert.h>
#include <orchMgr.h>
#include <util/stringutils.h>
#include <util/timeutils.h>
#include <net/BaseAsyncSvcHandler.h>

using namespace ::apache::thrift;  //NOLINT
using namespace ::apache::thrift::protocol;  //NOLINT
using namespace ::apache::thrift::transport;  //NOLINT
using namespace ::apache::thrift::server;  //NOLINT
using namespace ::apache::thrift::concurrency;  //NOLINT

using namespace  ::apis;  //NOLINT

namespace fds {
// class OrchMgr;

class ConfigurationServiceHandler : virtual public ConfigurationServiceIf {
    OrchMgr* om;
    kvstore::ConfigDB* configDB;

  public:
    explicit ConfigurationServiceHandler(OrchMgr* om) : om(om) {
        configDB = om->getConfigDB();
    }

    void apiException(std::string message, ErrorCode code = INTERNAL_SERVER_ERROR) {
        LOGERROR << "exception: " << message;
        ApiException e;
        e.message = message;
        e.errorCode    = code;
        throw e;
    }

    void checkDomainStatus() {
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        if (!domain->om_local_domain_up()) {
            apiException("local domain not up", SERVICE_NOT_READY);
        }
    }

    // stubs to keep cpp compiler happy - BEGIN
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
    void deleteVolume(const std::string& domainName, const std::string& volumeName) {}  //NOLINT
    void statVolume(VolumeDescriptor& _return, const std::string& domainName, const std::string& volumeName) {}  //NOLINT
    void listVolumes(std::vector<VolumeDescriptor> & _return, const std::string& domainName) {}  //NOLINT
    int32_t registerStream(const std::string& url, const std::string& http_method, const std::vector<std::string> & volume_names, const int32_t sample_freq_seconds, const int32_t duration_seconds) { return 0;} //NOLINT
    void getStreamRegistrations(std::vector<StreamingRegistrationMsg> & _return, const int32_t ignore) {} //NOLINT
    void deregisterStream(const int32_t registration_id) {}
    int64_t createSnapshotPolicy(const  ::FDS_ProtocolInterface::SnapshotPolicy& policy) {return 0;} //NOLINT
    void listSnapshotPolicies(std::vector< ::FDS_ProtocolInterface::SnapshotPolicy> & _return, const int64_t unused) {} //NOLINT
    void deleteSnapshotPolicy(const int64_t id) {} //NOLINT
    void attachSnapshotPolicy(const int64_t volumeId, const int64_t policyId) {} //NOLINT
    void listSnapshotPoliciesForVolume(std::vector< ::FDS_ProtocolInterface::SnapshotPolicy> & _return, const int64_t volumeId) {} //NOLINT
    void detachSnapshotPolicy(const int64_t volumeId, const int64_t policyId) {} //NOLINT
    void listVolumesForSnapshotPolicy(std::vector<int64_t> & _return, const int64_t policyId) {} //NOLINT
    void listSnapshots(std::vector< ::FDS_ProtocolInterface::Snapshot> & _return, const int64_t volumeId) {} //NOLINT
    void restoreClone(const int64_t volumeId, const int64_t snapshotId) {} //NOLINT
    int64_t cloneVolume(const int64_t volumeId, const int64_t fdsp_PolicyInfoId, const std::string& clonedVolumeName, const int64_t timelineTime) { return 0;} //NOLINT
    void createSnapshot(const int64_t volumeId, const std::string& snapshotName, const int64_t retentionTime, const int64_t timelineTime) {} //NOLINT

    // stubs to keep cpp compiler happy - END

    int64_t createTenant(boost::shared_ptr<std::string>& identifier) {
        return configDB->createTenant(*identifier);
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

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        VolumeInfo::pointer vol = volContainer->get_volume(*volumeName);
        Error err = volContainer->getVolumeStatus(*volumeName);
        if (err == ERR_OK) apiException("volume already exists", RESOURCE_ALREADY_EXISTS);

        fpi::FDSP_MsgHdrTypePtr header;
        fpi::FDSP_CreateVolTypePtr request;
        convert::getFDSPCreateVolRequest(header, request,
                                         *domainName, *volumeName, *volumeSettings);
        request->vol_info.tennantId = *tenantId;
        err = volContainer->om_create_vol(header, request, nullptr);
        if (err != ERR_OK) apiException("error creating volume");
    }

    int64_t getVolumeId(boost::shared_ptr<std::string>& volumeName) {
        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        VolumeInfo::pointer  vol = VolumeInfo::vol_cast_ptr(volContainer->rs_get_resource(volumeName->c_str())); //NOLINT
        if (vol) {
            return vol->rs_get_uuid().uuid_get_val();
        } else {
            LOGWARN << "unable to get volume info for vol:" << *volumeName;
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
            LOGWARN << "unable to get volume info for vol:" << *volumeId;
        }
    }

    void deleteVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName) {
        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        Error err = volContainer->getVolumeStatus(*volumeName);

        if (err != ERR_OK) apiException("volume does NOT exist", MISSING_RESOURCE);

        fpi::FDSP_MsgHdrTypePtr header;
        fpi::FDSP_DeleteVolTypePtr request;
        convert::getFDSPDeleteVolRequest(header, request, *domainName, *volumeName);
        err = volContainer->om_delete_vol(header, request);
    }

    void statVolume(VolumeDescriptor& volDescriptor,
                    boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName) {
        checkDomainStatus();
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        Error err = volContainer->getVolumeStatus(*volumeName);
        if (err != ERR_OK) apiException("volume NOT found", MISSING_RESOURCE);

        VolumeInfo::pointer  vol = volContainer->get_volume(*volumeName);

        convert::getVolumeDescriptor(volDescriptor, vol);
    }

    void listVolumes(std::vector<VolumeDescriptor> & _return,
                     boost::shared_ptr<std::string>& domainName) {
        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();

        LOGDEBUG << "just Active volumes";
        volContainer->vol_up_foreach<std::vector<VolumeDescriptor> &>(_return, [] (std::vector<VolumeDescriptor> &vec, VolumeInfo::pointer vol) { //NOLINT
                LOGDEBUG << (vol->vol_get_properties()->isSnapshot()?"snapshot":"volume")
                         << " - " << vol->vol_get_name()
                         << ":" << vol->vol_get_properties()->getStateName();
                VolumeDescriptor volDescriptor;
                // if (vol->vol_get_properties()->isStateActive()) {
                convert::getVolumeDescriptor(volDescriptor, vol);
                vec.push_back(volDescriptor);
                // }
            });
    }

    int32_t registerStream(boost::shared_ptr<std::string>& url,
                           boost::shared_ptr<std::string>& http_method,
                           boost::shared_ptr<std::vector<std::string> >& volume_names,
                           boost::shared_ptr<int32_t>& sample_freq_seconds,
                           boost::shared_ptr<int32_t>& duration_seconds) {
        int32_t regId = configDB->getNewStreamRegistrationId();
        fpi::StreamingRegistrationMsg regMsg;
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

    void getStreamRegistrations(std::vector<fpi::StreamingRegistrationMsg> & _return,
                                boost::shared_ptr<int32_t>& ignore) { //NOLINT
        configDB->getStreamRegistrations(_return);
    }

    void deregisterStream(boost::shared_ptr<int32_t>& registration_id) {
        configDB->removeStreamRegistration(*registration_id);
    }

    int64_t createSnapshotPolicy(boost::shared_ptr<fpi::SnapshotPolicy>& policy) {
        if (configDB->createSnapshotPolicy(*policy)) {
            om->snapshotMgr.addPolicy(*policy);
            return policy->id;
        }
        return -1;
    }

    void listSnapshotPolicies(std::vector<fpi::SnapshotPolicy> & _return,
                      boost::shared_ptr<int64_t>& unused) {
        configDB->listSnapshotPolicies(_return);
    }

    void deleteSnapshotPolicy(boost::shared_ptr<int64_t>& id) {
        configDB->deleteSnapshotPolicy(*id);
        om->snapshotMgr.removePolicy(*id);
    }

    void attachSnapshotPolicy(boost::shared_ptr<int64_t>& volumeId,
                              boost::shared_ptr<int64_t>& policyId) {
        configDB->attachSnapshotPolicy(*volumeId, *policyId);
    }

    void listSnapshotPoliciesForVolume(std::vector<fpi::SnapshotPolicy> & _return,
                                       boost::shared_ptr<int64_t>& volumeId) {
        configDB->listSnapshotPoliciesForVolume(_return, *volumeId);
    }

    void detachSnapshotPolicy(boost::shared_ptr<int64_t>& volumeId,
                              boost::shared_ptr<int64_t>& policyId) {
        configDB->detachSnapshotPolicy(*volumeId, *policyId);
    }

    void listVolumesForSnapshotPolicy(std::vector<int64_t> & _return,
                              boost::shared_ptr<int64_t>& policyId) {
        configDB->listVolumesForSnapshotPolicy(_return, *policyId);
    }

    void listSnapshots(std::vector<fpi::Snapshot> & _return,
                       boost::shared_ptr<int64_t>& volumeId) {
        configDB->listSnapshots(_return, *volumeId);
    }

    void restoreClone(boost::shared_ptr<int64_t>& volumeId,
                         boost::shared_ptr<int64_t>& snapshotId) {
    }

    int64_t cloneVolume(boost::shared_ptr<int64_t>& volumeId,
                        boost::shared_ptr<int64_t>& volPolicyId,
                        boost::shared_ptr<std::string>& clonedVolumeName,
                        boost::shared_ptr<int64_t>& timelineTime) {
        checkDomainStatus();
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        VolPolicyMgr      *volPolicyMgr = om->om_policy_mgr();
        VolumeInfo::pointer  parentVol, cloneVol;

        cloneVol = volContainer->get_volume(*clonedVolumeName);
        if (cloneVol != NULL) {
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
        desc.backupVolume = invalid_vol_id;
        desc.fSnapshot = false;
        desc.srcVolumeId = *volumeId;
        desc.timelineTime = *timelineTime;
        desc.createTime = util::getTimeStampMillis();

        if (parentVol->vol_get_properties()->lookupVolumeId == invalid_vol_id) {
            desc.lookupVolumeId = *volumeId;
        } else {
            desc.lookupVolumeId = parentVol->vol_get_properties()->lookupVolumeId;
        }

        desc.qosQueueId = invalid_vol_id;
        volPolicyMgr->fillVolumeDescPolicy(&desc);
        LOGDEBUG << "adding a clone request..";
        volContainer->addVolume(desc);
        return 0;
    }

    void createSnapshot(boost::shared_ptr<int64_t>& volumeId,
                        boost::shared_ptr<std::string>& snapshotName,
                        boost::shared_ptr<int64_t>& retentionTime,
                        boost::shared_ptr<int64_t>& timelineTime) {
                    // create the structure
        fpi::Snapshot snapshot;
        snapshot.snapshotName = util::strlower(*snapshotName);
        snapshot.volumeId = *volumeId;
        snapshot.snapshotId = configDB->getNewVolumeId();
        if (invalid_vol_id == snapshot.snapshotId) {
            LOGWARN << "unable to generate a new snapshot id";
            apiException("unable to generate a new snapshot id");
        }
        snapshot.snapshotPolicyId = 0;
        snapshot.creationTimestamp = util::getTimeStampMillis();
        snapshot.retentionTimeSeconds = *retentionTime;
        snapshot.timelineTime = *timelineTime;

        snapshot.state = fpi::ResourceState::Loading;
        LOGDEBUG << "snapshot request for volumeid:" << snapshot.volumeId
                 << " name:" << snapshot.snapshotName;


        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        fds::Error err = volContainer->addSnapshot(snapshot);
        if ( !err.ok() ) {
            LOGWARN << "snapshot add failed : " << err;
            apiException(err.GetErrstr());
        }
        // add this snapshot to the retention manager ...
        om->snapshotMgr.deleteScheduler->addSnapshot(snapshot);
    }
};

std::thread* runConfigService(OrchMgr* om) {
    int port = 9090;
    LOGNORMAL << "about to start config service @ " << port;
    boost::shared_ptr<ConfigurationServiceHandler> handler(new ConfigurationServiceHandler(om));  //NOLINT
    boost::shared_ptr<TProcessor> processor(new ConfigurationServiceProcessor(handler));  //NOLINT
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));  //NOLINT
    boost::shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory());
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());  //NOLINT

    // TODO(Andrew): Use a single OM processing thread for now...
    boost::shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(1);
    boost::shared_ptr<PosixThreadFactory> threadFactory = boost::shared_ptr<PosixThreadFactory>(
        new PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();

    // TNonblockingServer *server = new TNonblockingServer(processor,
    //                                                 protocolFactory,
    //                                                  port,
    //                                                  threadManager);
    TThreadedServer* server = new TThreadedServer(processor,
                                                  serverTransport,
                                                  transportFactory,
                                                  protocolFactory);
    return new std::thread ( [server] {
            LOGNOTIFY << "starting config service";
            server->serve();
            LOGCRITICAL << "stopping ... config service";
        });
}

}  // namespace fds

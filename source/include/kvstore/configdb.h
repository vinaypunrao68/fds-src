/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
#define SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
#include <unordered_set>
#include <map>
#include <string>
#include <vector>

#include <kvstore/kvstore.h>
#include <platform/node_services.h>
#include <fds_volume.h>
#include <dlt.h>
#include <fds_dmt.h>
#include <fdsp/config_types_types.h>
#include <exception>
#include <fds_subscription.h>
#include <fds_ldomain.h>
namespace fds {
struct node_data;

namespace kvstore {
using PolicyInfo = fpi::FDSP_PolicyInfoType;
using NodeInfoType = fpi::FDSP_RegisterNodeType;
struct ConfigException : std::runtime_error {
    explicit ConfigException(const std::string& what_arg) : std::runtime_error(what_arg) {}
};

struct ConfigDB : KVStore {

    typedef enum {
        CONFIGDB_EXCEPTION, // Exception reported by ConfigDB.
        NOT_FOUND,          // Successful ConfigDB access, but nothing found matching inquiry.
        NOT_UPDATED,        // Successful ConfigDB access, but requested updated could not be made.
        SUCCESS             // Successful ConfigDB access and object(s) to be updated or queried found.
    } ReturnType;

    ConfigDB(const std::string& host = "localhost", uint port = 6379, uint poolsize = 10);
    virtual ~ConfigDB();
    fds_uint64_t getLastModTimeStamp();

    // ConfigDB design version and upgrade management.
    void setConfigVersion();
    std::string getConfigVersion();
    bool isLatestConfigDBVersion(std::string& version);
    ReturnType upgradeConfigDBVersionLatest(std::string& currentVersion);

    // Global Domains
    std::string getGlobalDomain();
    bool setGlobalDomain(ConstString globalDomain= "fds");

    // Local Domains
    fds_ldomid_t getNewLocalDomainId();
    fds_ldomid_t putLocalDomain(const LocalDomain& localDomain, const bool isNew = true);
    fds_ldomid_t addLocalDomain(const LocalDomain& localDomain);
    ReturnType updateLocalDomain(const LocalDomain& localDomain);
    ReturnType deleteLocalDomain(fds_ldomid_t id);
    ReturnType localDomainExists(fds_ldomid_t id);
    ReturnType localDomainExists(const std::string& name);
    ReturnType getLocalDomainIds(std::vector<fds_ldomid_t>&ldomIds);
    fds_ldomid_t getLocalDomainId(const std::string& name);
    ReturnType listLocalDomains(std::vector<LocalDomain>& localDomains);
    ReturnType getLocalDomains(std::vector<LocalDomain>& localDomains);
    ReturnType getLocalDomain(fds_ldomid_t id, LocalDomain& localDomain);
    ReturnType getLocalDomain(const std::string& name, LocalDomain& localDomain);


    // ConfigDB management method for older versions of the ConfigDB.

    // Talc
    bool listLocalDomainsTalc(std::vector<fds::apis::LocalDomainDescriptorV07> &localDomains);

    // volumes
    fds_volid_t getNewVolumeId();
    bool setVolumeState(fds_volid_t volumeId, fpi::ResourceState state);
    bool addVolume(const VolumeDesc& volumeDesc);
    bool updateVolume(const VolumeDesc& volumeDesc);
    bool deleteVolume(fds_volid_t volumeId, int localDomain = 0);
    bool volumeExists(fds_volid_t volumeId);
    bool volumeExists(ConstString volumeName, int localDomain = 0);
    bool getVolumeIds(std::vector<fds_volid_t>& volumeIds, int localDomain = 0);
    bool getVolumes(std::vector<VolumeDesc>& volumes, int localDomain = 0);
    bool getVolume(fds_volid_t volumeId, VolumeDesc& volumeDesc);
    bool setVolumeSettings( long unsigned int volumeId, boost::shared_ptr<std::string> serialized );
    boost::shared_ptr<std::string>  getVolumeSettings( long unsigned int volumeId );

    // dlt
    // to store different types of dlt [current, new, old, target]
    fds_uint64_t getDltVersionForType(const std::string type, int localDomain = 0);
    bool setDltType(fds_uint64_t version, const std::string type, int localDomain = 0);

    bool storeDlt(const DLT& dlt, const std::string type = "" , int localDomain = 0);
    bool getDlt(DLT& dlt, fds_uint64_t version, int localDomain = 0);
    bool loadDlts(DLTManager& dltMgr, int localDomain = 0);
    bool storeDlts(DLTManager& dltMgr, int localDomain = 0);

    // dmt
    // to store different types of dmt [committed, target]
    fds_uint64_t getDmtVersionForType(const std::string type, int localDomain = 0);
    bool setDmtType(fds_uint64_t version, const std::string type, int localDomain = 0);

    bool storeDmt(const DMT& dmt, const std::string type = "" , int localDomain = 0);
    bool getDmt(DMT& dmt, fds_uint64_t version, int localDomain = 0);

    /*
     * TODO(Tinius) EPIC Restart - Full System Card FS-1355 
     * ( REPLACE WITH SERVICE MAP, defined below )
     * 
     * The following node methods should be cleaned up once all functionality
     * about nodes is moved to use service map defined below.
     */
    // nodes
    bool addNode(const NodeInfoType& node);
    bool updateNode(const NodeInfoType& node);
    bool removeNode(const NodeUuid& uuid);
    bool getNode(const NodeUuid& uuid, NodeInfoType& node); //NOLINT
    bool nodeExists(const NodeUuid& uuid);
    bool getNodeIds(std::unordered_set<NodeUuid, UuidHash>& nodes, int localDomain = 0);
    bool getAllNodes(std::vector<NodeInfoType>& nodes, int localDomain = 0);
    std::string getNodeName(const NodeUuid& uuid);

    bool getNodeServices(const NodeUuid& uuid, NodeServices& services);
    bool setNodeServices(const NodeUuid& uuid, const NodeServices& services);
    uint getNodeNameCounter();

    // service map
    bool deleteSvcMap(const fpi::SvcInfo& svcinfo);
    bool getSvcMap(std::vector<fpi::SvcInfo>& svcMap);
    bool updateSvcMap(const fpi::SvcInfo& svcinfo);
    bool changeStateSvcMap( const int64_t svc_uuid, 
                            const fpi::ServiceStatus svc_status );
    bool isPresentInSvcMap(const int64_t svc_uuid);
    /**
     * If service not found in configDB, returns SVC_STATUS_INVALID
     */
    fpi::ServiceStatus getStateSvcMap( const int64_t svc_uuid );

    // volume policies
    fds_uint32_t createQoSPolicy(const std::string& identifier,
                                 const fds_uint64_t minIops, const fds_uint64_t maxIops, const fds_uint32_t relPrio);
    fds_uint32_t getIdOfQoSPolicy(const std::string& identifier);
    bool getPolicy(fds_uint32_t volPolicyId, FDS_VolumePolicy& volumePolicy, int localDomain = 0); //NOLINT
    bool addPolicy(const FDS_VolumePolicy& volumePolicy, int localDomain = 0);
    bool updatePolicy(const FDS_VolumePolicy& volumePolicy, int localDomain = 0);
    bool deletePolicy(fds_uint32_t volPolicyId, int localDomain = 0);
    bool getPolicies(std::vector<FDS_VolumePolicy>& policies, int localDomain = 0);

    // stat streaming registrations
    int32_t getNewStreamRegistrationId();
    bool addStreamRegistration(apis::StreamingRegistrationMsg& streamReg);
    bool removeStreamRegistration(int regId);
    bool getStreamRegistration(int regId, apis::StreamingRegistrationMsg& streamReg);
    bool getStreamRegistrations(std::vector<apis::StreamingRegistrationMsg>& vecReg);

    // tenant stuff
    int64_t createTenant(const std::string& identifier);
    bool listTenants(std::vector<fds::apis::Tenant>& tenants);
    int64_t createUser(const std::string& identifier, const std::string& passwordHash, const std::string& secret, bool isAdmin); //NOLINT
    bool getUser(int64_t userId, fds::apis::User& user);
    bool listUsers(std::vector<fds::apis::User>& users);
    bool assignUserToTenant(int64_t userId, int64_t tenantId);
    bool revokeUserFromTenant(int64_t userId, int64_t tenantId);
    bool listUsersForTenant(std::vector<fds::apis::User>& users, int64_t tenantId);
    bool updateUser(int64_t  userId, const std::string& identifier, const std::string& passwordHash, const std::string& secret, bool isFdsAdmin); //NOLINT

    // snapshot
    bool createSnapshotPolicy(fds::apis::SnapshotPolicy& policy); //NOLINT
    bool getSnapshotPolicy(int64_t policyid, fds::apis::SnapshotPolicy& policy);
    bool listSnapshotPolicies(std::vector<fds::apis::SnapshotPolicy> & _return); //NOLINT
    bool deleteSnapshotPolicy(const int64_t id); //NOLINT
    bool attachSnapshotPolicy(fds_volid_t const volumeId, const int64_t policyId); //NOLINT
    bool listSnapshotPoliciesForVolume(std::vector<fds::apis::SnapshotPolicy> & _return, fds_volid_t const volumeId); //NOLINT
    bool detachSnapshotPolicy(fds_volid_t const volumeId, const int64_t policyId); //NOLINT
    bool listVolumesForSnapshotPolicy(std::vector<int64_t> & _return, const int64_t policyId); //NOLINT

    bool createSnapshot(fpi::Snapshot& snapshot);
    bool updateSnapshot(const fpi::Snapshot& snapshot);

    // volumeid & snapshotid should be set ...
    bool getSnapshot(fpi::Snapshot& snapshot);
    bool deleteSnapshot(fds_volid_t const volumeId, fds_volid_t const snapshotId);
    bool setSnapshotState(fpi::Snapshot& snapshot , fpi::ResourceState state);
    bool setSnapshotState(fds_volid_t const volumeId, fds_volid_t const snapshotId, fpi::ResourceState state); //NOLINT
    bool listSnapshots(std::vector<fpi::Snapshot> & _return, fds_volid_t const volumeId); //NOLINT

    bool setCapacityUsedNode( const int64_t svcUuid, const unsigned long usedCapacityInBytes );

    // Subscriptions
    fds_subid_t getNewSubscriptionId();
    ReturnType setSubscriptionState(fds_subid_t id, fpi::ResourceState state);
    fds_subid_t putSubscription(const Subscription &subscription, const bool isNew = true);
    ReturnType updateSubscription(const Subscription& subscription);
    ReturnType deleteSubscription(fds_subid_t id);
    ReturnType subscriptionExists(fds_subid_t id);
    ReturnType subscriptionExists(const std::string& name, const std::int64_t tenantId);
    ReturnType getSubscriptionIds(std::vector<fds_subid_t>& ids);
    fds_subid_t getSubscriptionId(const std::string& name, const std::int64_t tenantId);
    ReturnType getSubscriptions(std::vector<Subscription>& subscriptions);
    ReturnType getTenantSubscriptions(std::int64_t tenantId, std::vector<Subscription>& subscriptions);
    ReturnType getSubscription(fds_subid_t id, Subscription& subscription);
    ReturnType getSubscription(const std::string& name, const std::int64_t tenantId, Subscription& subscription);

  protected:
    void setModified();
    struct ModificationTracker {
        bool fModified = true;
        ConfigDB* cfgDB;
        explicit ModificationTracker(ConfigDB* cfgDB) : cfgDB(cfgDB) {}
        ~ModificationTracker() {
            if (fModified) {
            cfgDB->setModified();
            }
        }
    };
    
    void fromTo( fpi::SvcInfo svcInfo, kvstore::NodeInfoType nodeInfo );

  private:
    void setConfigVersion(const std::string& newVersion);
};

#define TRACKMOD(...) ModificationTracker modtracker(this)
#define NOMOD(...) modtracker.fModified = false
}  // namespace kvstore
}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_

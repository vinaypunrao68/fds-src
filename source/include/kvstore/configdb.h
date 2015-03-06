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
#include <fdsp/configuration_service_types.h>
#include <exception>
namespace fds {
struct node_data;

namespace kvstore {
using PolicyInfo = fpi::FDSP_PolicyInfoType;

struct ConfigException : std::runtime_error {
    explicit ConfigException(const std::string& what_arg) : std::runtime_error(what_arg) {}
};

struct ConfigDB : KVStore {
    ConfigDB(const std::string& host = "localhost", uint port = 6379, uint poolsize = 10);
    virtual ~ConfigDB();
    fds_uint64_t getLastModTimeStamp();
    fds_uint64_t getConfigVersion();

    // domains
    std::string getGlobalDomain();
    bool setGlobalDomain(ConstString globalDomain= "fds");
    bool addLocalDomain (ConstString name= "local", int localDomain = 0, int globalDomain = 0); //NOLINT
    bool getLocalDomains(std::map<int, std::string>& mapDomains, int globalDomain = 0);

    // volumes
    fds_uint64_t getNewVolumeId();
    bool setVolumeState(fds_volid_t volumeId, fpi::ResourceState state);
    bool addVolume(const VolumeDesc& volumeDesc);
    bool updateVolume(const VolumeDesc& volumeDesc);
    bool deleteVolume(fds_volid_t volumeId, int localDomain = 0);
    bool volumeExists(fds_volid_t volumeId);
    bool volumeExists(ConstString volumeName, int localDomain = 0);
    bool getVolumeIds(std::vector<fds_volid_t>& volumeIds, int localDomain = 0);
    bool getVolumes(std::vector<VolumeDesc>& volumes, int localDomain = 0);
    bool getVolume(fds_volid_t volumeId, VolumeDesc& volumeDesc);

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

    // nodes
    bool addNode(const struct node_data *node);
    bool updateNode(const struct node_data *node);
    bool removeNode(const NodeUuid& uuid);
    bool getNode(const NodeUuid& uuid, struct node_data *node);
    bool nodeExists(const NodeUuid& uuid);
    bool getNodeIds(std::unordered_set<NodeUuid, UuidHash>& nodes, int localDomain = 0);
    bool getAllNodes(std::vector<struct node_data>& nodes, int localDomain = 0);
    std::string getNodeName(const NodeUuid& uuid);

    bool getNodeServices(const NodeUuid& uuid, NodeServices& services);
    bool setNodeServices(const NodeUuid& uuid, const NodeServices& services);
    uint getNodeNameCounter();

    // volume policies
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
    bool attachSnapshotPolicy(const int64_t volumeId, const int64_t policyId); //NOLINT
    bool listSnapshotPoliciesForVolume(std::vector<fds::apis::SnapshotPolicy> & _return, const int64_t volumeId); //NOLINT
    bool detachSnapshotPolicy(const int64_t volumeId, const int64_t policyId); //NOLINT
    bool listVolumesForSnapshotPolicy(std::vector<int64_t> & _return, const int64_t policyId); //NOLINT

    bool createSnapshot(fpi::Snapshot& snapshot);
    bool updateSnapshot(const fpi::Snapshot& snapshot);

    // volumeid & snapshotid should be set ...
    bool getSnapshot(fpi::Snapshot& snapshot);
    bool deleteSnapshot(const int64_t volumeId, const int64_t snapshotId);
    bool setSnapshotState(fpi::Snapshot& snapshot , fpi::ResourceState state);
    bool setSnapshotState(const int64_t volumeId, const int64_t snapshotId, fpi::ResourceState state); //NOLINT
    bool listSnapshots(std::vector<fpi::Snapshot> & _return, const int64_t volumeId); //NOLINT

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
};

#define TRACKMOD(...) ModificationTracker modtracker(this)
#define NOMOD(...) modtracker.fModified = false
}  // namespace kvstore
}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_

/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef TESTLIB_FAKEDATASTORE_H_
#define TESTLIB_FAKEDATASTORE_H_


#include <orchMgr.h>

namespace fds
{
using NodeInfoType = fpi::FDSP_RegisterNodeType;
/**
 * @brief A fake data store
 * @details
 * This provides a basic implementation of
 * functions in configdb.cpp. In order to use custom
 * definitions of certain functions, simply inherit
 * from this class and override the function.
 * See SvcMapsUpdate_gtest.cpp to view example
 */
class FakeDataStore {

protected:
    std::vector<fpi::SvcInfo> fakeSvcMap;

    typedef enum {
        CONFIGDB_EXCEPTION, // Exception reported by ConfigDB.
        NOT_FOUND,          // Successful ConfigDB access, but nothing found matching inquiry.
        NOT_UPDATED,        // Successful ConfigDB access, but requested updated could not be made.
        SUCCESS             // Successful ConfigDB access and object(s) to be updated or queried found.
    } ReturnType;

public:

    FakeDataStore() { fakeSvcMap = {};}
    ~FakeDataStore() {}

    fds_uint64_t getLastModTimeStamp() { return 0; }

    /******************************************************************************
     *                      Local Domain Section
     *****************************************************************************/
    fds_ldomid_t getNewLocalDomainId() {
        return 0;
    }
    fds_ldomid_t putLocalDomain(const LocalDomain& localDomain, const bool isNew = true) {
        return 0;
    }
    fds_ldomid_t addLocalDomain(const LocalDomain& localDomain) {
        return 0;
    }
    kvstore::ConfigDB::ReturnType updateLocalDomain(const LocalDomain& localDomain) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    kvstore::ConfigDB::ReturnType deleteLocalDomain(fds_ldomid_t id) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    kvstore::ConfigDB::ReturnType localDomainExists(fds_ldomid_t id) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType localDomainExists(const std::string& name) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getLocalDomainIds(std::vector<fds_ldomid_t>&ldomIds) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    fds_ldomid_t getLocalDomainId(const std::string& name) {
        return 0;
    }
    kvstore::ConfigDB::ReturnType listLocalDomains(std::vector<LocalDomain>& localDomains) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getLocalDomains(std::vector<LocalDomain>& localDomains) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getLocalDomain(fds_ldomid_t id, LocalDomain& localDomain) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getLocalDomain(const std::string& name, LocalDomain& localDomain) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }

    /******************************************************************************
     *                      Volume Section
     *****************************************************************************/
    fds_volid_t getNewVolumeId() {
        static int v = 0;
        return fds::fds_value_type<uint64_t>(v);
    }
    bool setVolumeState(fds_volid_t volumeId, fpi::ResourceState state) {
        return false;
    }
    bool addVolume(const VolumeDesc& volumeDesc) {
        return false;
    }
    bool updateVolume(const VolumeDesc& volumeDesc) {
        return false;
    }
    bool deleteVolume(fds_volid_t volumeId, int localDomain = 0) {
        return false;
    }
    bool volumeExists(fds_volid_t volumeId) {
        return false;
    }
    bool volumeExists(ConstString volumeName, int localDomain = 0) {
        return false;
    }
    bool getVolumeIds(std::vector<fds_volid_t>& volumeIds, int localDomain = 0) {
        return false;
    }
    bool getVolumes(std::vector<VolumeDesc>& volumes, int localDomain = 0) {
        return false;
    }
    bool getVolume(fds_volid_t volumeId, VolumeDesc& volumeDesc) {
        return false;
    }

    bool setVolumeSettings( long unsigned int volumeId, boost::shared_ptr<std::string> serialized ) {
        return false;
    }

    boost::shared_ptr<std::string>  getVolumeSettings( long unsigned int volumeId ) {
        std::string s;
        return boost::make_shared<std::string>(s);
    }

    /******************************************************************************
     *                      DLT/DMT Section
     *****************************************************************************/

    fds_uint64_t getDltVersionForType(const std::string type, int localDomain = 0) {
        return 0;
    }
    bool setDltType(fds_uint64_t version, const std::string type, int localDomain = 0) {
        return false;
    }

    bool storeDlt(const DLT& dlt, const std::string type = "" , int localDomain = 0) {
        return false;
    }

    bool getDlt(DLT& dlt, fds_uint64_t version, int localDomain = 0) {
        return false;
    }

    bool loadDlts(DLTManager& dltMgr, int localDomain = 0) {
        return false;
    }

    bool storeDlts(DLTManager& dltMgr, int localDomain = 0) {
        return false;
    }

    fds_uint64_t getDmtVersionForType(const std::string type, int localDomain = 0) {
        return 0;
    }
    bool setDmtType(fds_uint64_t version, const std::string type, int localDomain = 0) {
        return false;
    }

    bool storeDmt(const DMT& dmt, const std::string type = "" , int localDomain = 0) {
        return false;
    }
    bool getDmt(DMT& dmt, fds_uint64_t version, int localDomain = 0) {
        return false;
    }

    /******************************************************************************
     *                      Node Section
     *****************************************************************************/
    bool addNode(const NodeInfoType& node) {
        return false;
    }

    bool updateNode(const NodeInfoType& node) {
        return false;
    }

    bool removeNode(const NodeUuid& uuid) {
        return false;
    }

    bool getNode(const NodeUuid& uuid, NodeInfoType& node) {
        return false;
    }

    bool nodeExists(const NodeUuid& uuid) {
        return false;
    }

    bool getNodeIds(std::unordered_set<NodeUuid, UuidHash>& nodes, int localDomain = 0) {
        return false;
    }

    bool getAllNodes(std::vector<NodeInfoType>& nodes, int localDomain = 0) {
        return false;
    }

    std::string getNodeName(const NodeUuid& uuid) {
        std::string s;
        return s;
    }

    bool getNodeServices(const NodeUuid& uuid, NodeServices& services) {
        return false;
    }

    bool setNodeServices(const NodeUuid& uuid, const NodeServices& services) {
        return false;
    }

    uint getNodeNameCounter() {
        return 0;
    }

    /******************************************************************************
     *                      Service Map Section
     *****************************************************************************/
    bool getSvcMap(std::vector<fpi::SvcInfo>& svcMap) {
        return false;
    }

    bool getSvcInfo(const fds_uint64_t svc_uuid, fpi::SvcInfo& svcInfo) {
        return false;
    }

    bool changeStateSvcMap( fpi::SvcInfoPtr svcInfoPtr) {
        return false;
    }

    bool deleteSvcMap(const fpi::SvcInfo& svcInfo) {
        return false;
    }

    bool updateSvcMap(const fpi::SvcInfo& svcinfo) {
        return false;
    }

    bool isPresentInSvcMap(const int64_t svc_uuid) {
        return false;
    }

    fpi::ServiceStatus getStateSvcMap( const int64_t svc_uuid ) {
        return fpi::SVC_STATUS_INVALID;
    }
    /******************************************************************************
     *                      Volume Policy Section
     *****************************************************************************/
    fds_uint32_t createQoSPolicy(const std::string& identifier,
                                 const fds_uint64_t minIops, const fds_uint64_t maxIops, const fds_uint32_t relPrio);
    fds_uint32_t getIdOfQoSPolicy(const std::string& identifier) {
        return 0;
    }
    bool getPolicy(fds_uint32_t volPolicyId, FDS_VolumePolicy& volumePolicy, int localDomain = 0) { //NOLINT
        return false;
    }
    bool addPolicy(const FDS_VolumePolicy& volumePolicy, int localDomain = 0) {
        return false;
    }
    bool updatePolicy(const FDS_VolumePolicy& volumePolicy, int localDomain = 0) {
        return false;
    }
    bool deletePolicy(fds_uint32_t volPolicyId, int localDomain = 0) {
        return false;
    }
    bool getPolicies(std::vector<FDS_VolumePolicy>& policies, int localDomain = 0) {
        return false;
    }

    /******************************************************************************
     *                      Stat Stream Registrations Section
     *****************************************************************************/
    int32_t getNewStreamRegistrationId() {
        return 0;
    }
    bool addStreamRegistration(apis::StreamingRegistrationMsg& streamReg) {
        return false;
    }
    bool removeStreamRegistration(int regId) {
        return false;
    }
    bool getStreamRegistration(int regId, apis::StreamingRegistrationMsg& streamReg) {
        return false;
    }
    bool getStreamRegistrations(std::vector<apis::StreamingRegistrationMsg>& vecReg) {
        return false;
    }

    /******************************************************************************
     *                      Tenant Section
     *****************************************************************************/
    int64_t createTenant(const std::string& identifier) {
        return 0;
    }
    bool listTenants(std::vector<fds::apis::Tenant>& tenants) {
        return false;
    }
    int64_t createUser(const std::string& identifier, const std::string& passwordHash, const std::string& secret, bool isAdmin) { //NOLINT
        static int64_t u = 0;
        return ++u;
    }
    bool getUser(int64_t userId, fds::apis::User& user) {
        return false;
    }
    bool listUsers(std::vector<fds::apis::User>& users) {
        return false;
    }
    bool assignUserToTenant(int64_t userId, int64_t tenantId) {
        return false;
    }
    bool revokeUserFromTenant(int64_t userId, int64_t tenantId) {
        return false;
    }
    bool listUsersForTenant(std::vector<fds::apis::User>& users, int64_t tenantId) {
        return false;
    }
    bool updateUser(int64_t  userId, const std::string& identifier, const std::string& passwordHash, const std::string& secret, bool isFdsAdmin) { //NOLINT
        return false;
    }

    /******************************************************************************
     *                      Snapshot Section
     *****************************************************************************/
    bool createSnapshotPolicy(fds::apis::SnapshotPolicy& policy) { //NOLINT
        return false;
    }
    bool getSnapshotPolicy(int64_t policyid, fds::apis::SnapshotPolicy& policy) {
        return false;
    }
    bool listSnapshotPolicies(std::vector<fds::apis::SnapshotPolicy> & _return) { //NOLINT
        return false;
    }
    bool deleteSnapshotPolicy(const int64_t id) { //NOLINT
        return false;
    }
    bool attachSnapshotPolicy(fds_volid_t const volumeId, const int64_t policyId) { //NOLINT
        return false;
    }
    bool listSnapshotPoliciesForVolume(std::vector<fds::apis::SnapshotPolicy> & _return, fds_volid_t const volumeId) { //NOLINT
        return false;
    }
    bool detachSnapshotPolicy(fds_volid_t const volumeId, const int64_t policyId) { //NOLINT
        return false;
    }
    bool listVolumesForSnapshotPolicy(std::vector<int64_t> & _return, const int64_t policyId) { //NOLINT
        return false;
    }

    bool createSnapshot(fpi::Snapshot& snapshot) {
        return false;
    }
    bool updateSnapshot(const fpi::Snapshot& snapshot) {
        return false;
    }

    // volumeid & snapshotid should be set ...
    bool getSnapshot(fpi::Snapshot& snapshot) {
        return false;
    }
    bool deleteSnapshot(fds_volid_t const volumeId, fds_volid_t const snapshotId) {
        return false;
    }
    bool setSnapshotState(fpi::Snapshot& snapshot , fpi::ResourceState state) {
        return false;
    }
    bool setSnapshotState(fds_volid_t const volumeId, fds_volid_t const snapshotId, fpi::ResourceState state) { //NOLINT
        return false;
    }
    bool listSnapshots(std::vector<fpi::Snapshot> & _return, fds_volid_t const volumeId) { //NOLINT
        return false;
    }

    /******************************************************************************
     *                      Subscription Section
     *****************************************************************************/
    fds_subid_t getNewSubscriptionId() {
        return 0;
    }
    kvstore::ConfigDB::ReturnType setSubscriptionState(fds_subid_t id, fpi::ResourceState state) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    fds_subid_t putSubscription(const Subscription &subscription, const bool isNew = true) {
        return 0;
    }
    kvstore::ConfigDB::ReturnType updateSubscription(const Subscription& subscription) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    kvstore::ConfigDB::ReturnType deleteSubscription(fds_subid_t id) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    kvstore::ConfigDB::ReturnType subscriptionExists(fds_subid_t id) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType subscriptionExists(const std::string& name, const std::int64_t tenantId) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getSubscriptionIds(std::vector<fds_subid_t>& ids) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    fds_subid_t getSubscriptionId(const std::string& name, const std::int64_t tenantId) {
        return 0;
    }
    kvstore::ConfigDB::ReturnType getSubscriptions(std::vector<Subscription>& subscriptions) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getTenantSubscriptions(std::int64_t tenantId, std::vector<Subscription>& subscriptions) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getSubscription(fds_subid_t id, Subscription& subscription) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getSubscription(const std::string& name, const std::int64_t tenantId, Subscription& subscription) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }


    // Other functions
    bool setCapacityUsedNode( const int64_t svcUuid, const unsigned long usedCapacityInBytes ) {
        return false;
    }

    bool listLocalDomainsTalc(std::vector<fds::apis::LocalDomainDescriptorV07> &localDomains) {
        return false;
    }

    // ConfigDB design version and upgrade management.
    void setConfigVersion() { }

    std::string getConfigVersion() {
        std::string s;
        return s;
    }

    bool isLatestConfigDBVersion(std::string& version) {
        return false;
    }

    ReturnType upgradeConfigDBVersionLatest(std::string& currentVersion);

    // Global Domains
    std::string getGlobalDomain() {
        std::string s;
        return s;
    }

    bool setGlobalDomain(ConstString globalDomain= "fds") {
        return false;
    }
};

#endif // TESTLIB_FAKEDATASTORE_H_
} // namespace fds

/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <orchMgr.h>
#include <orch-mgr/om-service.h>

// UUT
#include <orch-mgr/configservice/configservice.cpp>

namespace fds {

extern OrchMgr *orchMgr;
OM_Module *omModule;

OM_Module *OM_Module::om_singleton()
{
    return omModule;
}

/**
 * @brief A fake process
 * @details
 * The ConfigurationServiceHandler is currently coded to fetch the Config
 * data store from the process for many operations. Given that dependency,
 * this test is more like a component test than a unit test.
 */
class FakeProcess : public FdsProcess {

private:
    FdsConfigAccessor configAccess_;

public:
    FakeProcess();
    virtual ~FakeProcess() {}
    virtual int run() { return 0; }
    virtual FdsConfigAccessor get_conf_helper() const override {
        return configAccess_;
    }
    virtual boost::shared_ptr<FdsConfig> get_fds_config() const {
        return configAccess_.get_fds_config();
    }
};

FakeProcess::FakeProcess() : FdsProcess() {

    g_fdsprocess = this;

    // Dependency on global logger
    std::string strPath = "/fds/";
    FdsProcess::proc_root = new FdsRootDir(strPath);

    boost::shared_ptr<FdsConfig> config(new FdsConfig());
    configAccess_.init(config, "fds.");

    // We are not going to create a sig handler thread, so without
    // the following the parent destructor waits indefinitely.
    FdsProcess::sig_tid_ready = true;
    FdsProcess::sig_tid_stop_cv_.notify_all();
}

/**
 * @brief A fake data store
 * @details
 * Inherits from no other, but must define interfaces used by
 * ConfigurationServiceHandler<DataStoreT>.
 */
class FakeDataStore {

public:

    fds_uint64_t getLastModTimeStamp() { return 0; }

    // Local Domains
    fds_ldomid_t getNewLocalDomainId() {
        static fds_ldomid_t d = 0;
        return ++d;
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

    // volumes
    fds_volid_t getNewVolumeId() {
        static int v = 0;
        return fds::fds_value_type<uint64_t>(++v);
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

    // service map
    bool getSvcMap(std::vector<fpi::SvcInfo>& svcMap) {
        return false;
    }

    // volume policies
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

    // stat streaming registrations
    int32_t getNewStreamRegistrationId() {
        static int32_t i = 0;
        return ++i;
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

    // tenant stuff
    int64_t createTenant(const std::string& identifier) {
        static int64_t t = 0;
        return ++t;
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

    // snapshot
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

    // Subscriptions
    fds_subid_t getNewSubscriptionId() {
        static fds_subid_t s = 0;
        return ++s;
    }
    kvstore::ConfigDB::ReturnType setSubscriptionState(fds_subid_t id, fpi::ResourceState state) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    fds_subid_t putSubscription(const Subscription &subscription, const bool isNew = true) {
        static fds_subid_t s = 0;
        return ++s;
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
};

} // namespace fds

TEST(CreateSubscription, FeatureToggle) {
    // FDS process depends on global logger. This dependency prevents true
    // isolation testing.
    // Initialize logging (logptr, g_fdslog).
    // Host volume must have permissions in /fds to log test.
    fds::fds_log *temp_log = new fds::fds_log("fds", "logs");
    temp_log->setSeverityFilter(
        fds::fds_log::getLevelFromName("DEBUG"));
    fds::g_fdslog = temp_log;
    GLOGNORMAL << "configservice_gtest";
    // Supplies the configuration datastore
    fds::FakeProcess fakeProcess;
    // Fake subscription data
    fds::FakeDataStore dataStore;

    // Yes, that global orchMgr is nullptr. Look out!
    fds::apis::ConfigurationServiceHandler<fds::FakeDataStore> uut(fds::orchMgr);
    uut.setConfigDB(&dataStore);

    std::string strName("Subscription1");
    int64_t tenantID = 6;

    // prepare arguments
    auto pArg1 = boost::make_shared<std::string>(strName);
    auto pArg2 = boost::make_shared<int64_t>(tenantID);
    auto pArg3 = boost::make_shared<int32_t>(0);
    auto pArg6 = boost::make_shared< ::fds::apis::SubscriptionType>( ::fds::apis::SubscriptionType::CONTENT_DELTA);
    auto pArg7 = boost::make_shared< ::fds::apis::SubscriptionScheduleType>( ::fds::apis::SubscriptionScheduleType::TIME);
    auto pArg8 = boost::make_shared<int64_t>(30000);

    // Should throw, since subscriptions are not enabled
    EXPECT_THROW(uut.createSubscription(pArg1, pArg2, pArg3,
        pArg2, pArg3,
        pArg6, pArg7, pArg8), fpi::ApiException);

    // Now turn on the feature toggle
    boost::shared_ptr<fds::FdsConfig> config = fakeProcess.get_fds_config();
    config->set("fds.feature_toggle.common.enable_subscriptions", true);

    // Verify feature enabled
    fds::FdsConfigAccessor configAccess(config, "fds.feature_toggle.");
    bool result = configAccess.get<bool>("common.enable_subscriptions", false);
    EXPECT_EQ(result, true);

    // Yes, that global orchMgr is nullptr. Look out!
    fds::apis::ConfigurationServiceHandler<fds::FakeDataStore> uut1(fds::orchMgr);
    uut1.setConfigDB(&dataStore);

    EXPECT_NO_THROW(uut1.createSubscription(pArg1, pArg2, pArg3,
        pArg2, pArg3,
        pArg6, pArg7, pArg8));
}

int main(int argc, char* argv[]) {

    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}


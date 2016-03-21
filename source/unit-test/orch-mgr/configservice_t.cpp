/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <orchMgr.h>
#include <orch-mgr/om-service.h>
#include <testlib/FakeDataStore.h>

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


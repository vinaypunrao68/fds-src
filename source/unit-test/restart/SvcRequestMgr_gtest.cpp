/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <boost/make_shared.hpp>
#include <fds_module_provider.h>
#include <net/net_utils.h>
#include <net/SvcRequestPool.h>
#include <net/SvcMgr.h>
#include <fdsp_utils.h>
// #include <ObjectId.h>
#include <fiu-control.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include <util/fiu_util.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/profiler.h>
// #include "IThreadpool.h"

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

struct SvcMgrModuleProvider : CommonModuleProviderIf {
    SvcMgrModuleProvider(const std::string &configFile, long long uuid, int port) {
        /* config */
        std::string configBasePath = "fds.dm.";
        boost::shared_ptr<FdsConfig> config(new FdsConfig(configFile, 0, {nullptr}));
        configHelper_.init(config, configBasePath);
        config->set("fds.dm.svc.uuid", uuid);
        config->set("fds.dm.svc.port", port);

        /* timer */
        timer_.reset(new FdsTimer());

        /* counters */
        cntrsMgr_.reset(new FdsCountersMgr(configBasePath));

        /* threadpool */
        threadpool_.reset(new fds_threadpool(3));

        /* service mgr */
        auto handler = boost::make_shared<PlatNetSvcHandler>(this);
        auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);
        fpi::SvcInfo svcInfo;
        svcInfo.svc_id.svc_uuid.svc_uuid = 
            static_cast<int64_t>(configHelper_.get<long long>("svc.uuid"));
        svcInfo.ip = net::get_local_ip(configHelper_.get_abs<std::string>("fds.nic_if"));
        svcInfo.svc_port = configHelper_.get<int>("svc.port");
        svcInfo.incarnationNo = util::getTimeStampSeconds();
        svcMgr_.reset(new SvcMgr(this, handler, processor, svcInfo));
        svcMgr_->mod_init(nullptr);
    }

    virtual FdsConfigAccessor get_conf_helper() const override {
        return configHelper_;
    }

    virtual boost::shared_ptr<FdsConfig> get_fds_config() const override {
        return configHelper_.get_fds_config();
    }

    virtual FdsTimerPtr getTimer() const override { return timer_; }

    virtual boost::shared_ptr<FdsCountersMgr> get_cntrs_mgr() const override { return cntrsMgr_; }

    virtual fds_threadpool *proc_thrpool() const override { return threadpool_.get(); }

    virtual SvcMgr* getSvcMgr() override { return svcMgr_.get(); }

 protected:
    FdsConfigAccessor configHelper_;
    FdsTimerPtr timer_;
    boost::shared_ptr<FdsCountersMgr> cntrsMgr_;
    fds_threadpoolPtr threadpool_;
    boost::shared_ptr<SvcMgr> svcMgr_;
};
using SvcMgrModuleProviderPtr = boost::shared_ptr<SvcMgrModuleProvider>;

struct SvcRequestMgrTest : BaseTestFixture {
    SvcRequestMgrTest() {
    }

    static void SetUpTestCase() {
        uint64_t svcUuidCntr = 0x100;
        int portCntr = 10000;
        std::vector<fpi::SvcInfo> svcMap;

        /* Create svc mgr instances */
        svcMgrProviders.resize(3);
        for (uint32_t i = 0; i < svcMgrProviders.size(); i++) {
            svcMgrProviders[i] = boost::make_shared<SvcMgrModuleProvider>(
                "/fds/etc/platform.conf",svcUuidCntr, portCntr);

            auto svcMgr = svcMgrProviders[i]->getSvcMgr();
            svcMap.push_back(svcMgr->getSelfSvcInfo());

            svcUuidCntr++;
            portCntr++;

        }

        /* Update the service map on every svcMgr instance */
        for (uint32_t i = 0; i < svcMgrProviders.size(); i++) {
            auto svcMgr = svcMgrProviders[i]->getSvcMgr();
            svcMap.push_back(svcMgr->getSelfSvcInfo());
            svcMgr->updateSvcMap(svcMap);
        }
    }

    static void TearDownTestCase() {
    }

    static std::vector<SvcMgrModuleProviderPtr> svcMgrProviders;
};
std::vector<SvcMgrModuleProviderPtr> SvcRequestMgrTest::svcMgrProviders;

/**
* @brief Test for basic EPSvcRequest
*/
TEST_F(SvcRequestMgrTest, epsvcrequest)
{
    auto svcMgr1 = svcMgrProviders[0]->getSvcMgr();
    auto svcMgr2 = svcMgrProviders[1]->getSvcMgr();

    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = svcMgr1->getSvcRequestMgr()->newEPSvcRequest(svcMgr2->getSelfSvcUuid());
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onResponseCb(svcStatusWaiter.cb);
    asyncReq->invoke();

    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_OK) << "Error: " << svcStatusWaiter.error;
    ASSERT_EQ(svcStatusWaiter.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter.response->status;
}

TEST_F(SvcRequestMgrTest, failoversvcrequest)
{
    ASSERT_TRUE(svcMgrProviders.size() >= 3);

    auto svcMgr1 = svcMgrProviders[0]->getSvcMgr();
    auto svcMgr2 = svcMgrProviders[1]->getSvcMgr();
    auto svcMgr3 = svcMgrProviders[2]->getSvcMgr();

    DltTokenGroupPtr tokGroup = boost::make_shared<DltTokenGroup>(2);
    tokGroup->set(0, NodeUuid(svcMgr2->getSelfSvcUuid()));
    tokGroup->set(1, NodeUuid(svcMgr3->getSelfSvcUuid()));
    auto epProvider = boost::make_shared<DltObjectIdEpProvider>(tokGroup);

    SvcRequestCbTask<FailoverSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = svcMgr1->getSvcRequestMgr()->newFailoverSvcRequest(epProvider);
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onResponseCb(svcStatusWaiter.cb);
    asyncReq->invoke();

    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_OK) << "Error: " << svcStatusWaiter.error;
    ASSERT_EQ(svcStatusWaiter.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter.response->status;
}

TEST_F(SvcRequestMgrTest, quorumsvcrequest)
{
    ASSERT_TRUE(svcMgrProviders.size() >= 3);

    auto svcMgr1 = svcMgrProviders[0]->getSvcMgr();
    auto svcMgr2 = svcMgrProviders[1]->getSvcMgr();
    auto svcMgr3 = svcMgrProviders[2]->getSvcMgr();

    DltTokenGroupPtr tokGroup = boost::make_shared<DltTokenGroup>(2);
    tokGroup->set(0, NodeUuid(svcMgr2->getSelfSvcUuid()));
    tokGroup->set(1, NodeUuid(svcMgr3->getSelfSvcUuid()));
    auto epProvider = boost::make_shared<DltObjectIdEpProvider>(tokGroup);

    SvcRequestCbTask<QuorumSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = svcMgr1->getSvcRequestMgr()->newQuorumSvcRequest(epProvider);
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onResponseCb(svcStatusWaiter.cb);
    asyncReq->invoke();

    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_OK) << "Error: " << svcStatusWaiter.error;
    ASSERT_EQ(svcStatusWaiter.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter.response->status;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
#if 0
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("smuuid", po::value<uint64_t>()->default_value(0), "smuuid")
        ("puts-cnt", po::value<int>(), "puts count")
        ("profile", po::value<bool>()->default_value(false), "google profile")
        ("failsends-before", po::value<bool>()->default_value(false), "fail sends before")
        ("failsends-after", po::value<bool>()->default_value(false), "fail sends after")
        ("uturn-asyncreqt", po::value<bool>()->default_value(false), "uturn async reqt")
        ("uturn", po::value<bool>()->default_value(false), "uturn")
        ("disable-schedule", po::value<bool>()->default_value(false), "disable scheduling");
    SMApi::init(argc, argv, opts);
#endif
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message");
    SvcRequestMgrTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

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
#include <fiu-control.h>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include <util/fiu_util.h>
#include <SvcMgrModuleProvider.hpp>
#include <FakeSvcDomain.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/profiler.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

/**
* @brief Tests svc map update in the domain.
*/
TEST(SvcRequestMgr, epsvcrequest) {
    int cnt = 2;
    FakeSyncSvcDomain domain(cnt);

    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    domain.sendGetStatusEpSvcRequest(0, 1, svcStatusWaiter);
    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_OK) << "Error: " << svcStatusWaiter.error;
    ASSERT_EQ(svcStatusWaiter.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter.response->status;
}

/**
* @brief Test sending endpoint request agains invalid endpoint
*
*/
TEST(SvcRequestMgr, epsvcrequest_invalidep)
{
    int cnt = 2;
    FakeSyncSvcDomain domain(cnt);

    auto svcMgr1 = domain[0]->getSvcMgr();

    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = svcMgr1->getSvcRequestMgr()->newEPSvcRequest(FakeSvcDomain::INVALID_SVCUUID);
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onResponseCb(svcStatusWaiter.cb);
    asyncReq->invoke();

    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_SVC_REQUEST_INVOCATION) << "Error: " << svcStatusWaiter.error;
}

/* Tests basic failover style request */
TEST(SvcRequestMgr, failoversvcrequest)
{
    int cnt = 3;
    FakeSyncSvcDomain domain(cnt);

    SvcRequestCbTask<FailoverSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    domain.sendGetStatusFailoverSvcRequest(0, {1,2}, svcStatusWaiter);

    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_OK) << "Error: " << svcStatusWaiter.error;
    ASSERT_EQ(svcStatusWaiter.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter.response->status;
}

/* Tests basic quorum reqesut */
TEST(SvcRequestMgr, quorumsvcrequest)
{
    int cnt = 3;
    FakeSyncSvcDomain domain(cnt);

    SvcRequestCbTask<QuorumSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    domain.sendGetStatusQuorumSvcRequest(0, {1,2}, svcStatusWaiter);

    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_OK) << "Error: " << svcStatusWaiter.error;
    ASSERT_EQ(svcStatusWaiter.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter.response->status;
}

#if 0
struct SvcRequestMgrTest : BaseTestFixture {
    SvcRequestMgrTest() {
    }

    static void SetUpTestCase() {
        uint64_t svcUuidCntr = 0x100;
        int portCntr = 10000;
        std::vector<fpi::SvcInfo> svcMap;

        invalidSvcUuid.svc_uuid = svcUuidCntr - 1;

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

    static void sendGetStatusEpSvcRequest(SvcMgr* fromMgr, SvcMgr* toMgr,
                                   SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> &cbHandle)
    {
        auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
        auto asyncReq = fromMgr->getSvcRequestMgr()->newEPSvcRequest(toMgr->getSelfSvcUuid());
        asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
        asyncReq->setTimeoutMs(1000);
        asyncReq->onResponseCb(cbHandle.cb);
        asyncReq->invoke();
    }

    static void TearDownTestCase() {
    }

    static std::vector<SvcMgrModuleProviderPtr> svcMgrProviders;
    static fpi::SvcUuid invalidSvcUuid;
};
std::vector<SvcMgrModuleProviderPtr> SvcRequestMgrTest::svcMgrProviders;
fpi::SvcUuid SvcRequestMgrTest::invalidSvcUuid;

/**
* @brief Test for basic EPSvcRequest
*/
TEST_F(SvcRequestMgrTest, epsvcrequest)
{
    auto svcMgr1 = svcMgrProviders[0]->getSvcMgr();
    auto svcMgr2 = svcMgrProviders[1]->getSvcMgr();

    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    sendGetStatusEpSvcRequest(svcMgr1, svcMgr2, svcStatusWaiter);
    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_OK) << "Error: " << svcStatusWaiter.error;
    ASSERT_EQ(svcStatusWaiter.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter.response->status;
}

/**
* @brief Test sending endpoint request agains invalid endpoint
*
*/
TEST_F(SvcRequestMgrTest, epsvcrequest_invalidep)
{
    auto svcMgr1 = svcMgrProviders[0]->getSvcMgr();

    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = svcMgr1->getSvcRequestMgr()->newEPSvcRequest(invalidSvcUuid);
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onResponseCb(svcStatusWaiter.cb);
    asyncReq->invoke();

    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_SVC_REQUEST_INVOCATION) << "Error: " << svcStatusWaiter.error;
}

/**
* @brief Test sending endpoint request agains invalid endpoint
*/
TEST_F(SvcRequestMgrTest, epsvcrequest_downep)
{
    auto svcMgr1 = svcMgrProviders[0]->getSvcMgr();
    auto svcMgr2 = svcMgrProviders[1]->getSvcMgr();

    svcMgr2->stopServer();

    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> cbWaiter1;
    sendGetStatusEpSvcRequest(svcMgr1, svcMgr2, cbWaiter1);
    cbWaiter1.await();
    ASSERT_EQ(cbWaiter1.error, ERR_SVC_REQUEST_TIMEOUT) << "Error: " << cbWaiter1.error;

    svcMgr2->startServer();

    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> cbWaiter2;
    sendGetStatusEpSvcRequest(svcMgr1, svcMgr2, cbWaiter2);
    cbWaiter2.await();
    ASSERT_EQ(cbWaiter2.error, ERR_SVC_REQUEST_INVOCATION) << "Error: " << cbWaiter2.error;
}
#endif

#if 0
/* Tests basic failover style request */
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

/* Tests basic quorum reqesut */
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
#endif

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
#if 0
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message");
    SvcRequestMgrTest::init(argc, argv, opts);
#endif
    return RUN_ALL_TESTS();
}

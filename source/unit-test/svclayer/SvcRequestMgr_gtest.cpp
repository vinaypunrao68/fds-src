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
#include <testlib/FakeSvcDomain.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testlib/TestFixtures.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

struct SvcRequestMgrTest : BaseTestFixture {
    static std::string confFile;

    static void SetUpTestCase() {
        confFile = getArg<std::string>("fds-root") + std::string("/etc/platform.conf");
    }

};
std::string SvcRequestMgrTest::confFile;

/**
* @brief Tests svc map update in the domain.
*/
TEST_F(SvcRequestMgrTest, epsvcrequest)
{
    int cnt = 3;
    FakeSyncSvcDomain domain(cnt, confFile);

    /* dest service is up...request should succeed */
    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter1;
    domain.sendGetStatusEpSvcRequest(1, 2, svcStatusWaiter1);
    svcStatusWaiter1.await();
    ASSERT_EQ(svcStatusWaiter1.error, ERR_OK) << "Error: " << svcStatusWaiter1.error;
    ASSERT_EQ(svcStatusWaiter1.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter1.response->status;

    /* dest service is down...request should fail */
    domain.kill(2);
    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter2;
    domain.sendGetStatusEpSvcRequest(1, 2, svcStatusWaiter2);
    svcStatusWaiter2.await();
    ASSERT_TRUE(svcStatusWaiter2.error == ERR_SVC_REQUEST_INVOCATION ||
                svcStatusWaiter2.error == ERR_SVC_REQUEST_TIMEOUT)
        << "Error: " << svcStatusWaiter2.error;

    /* dest service is up again...request should succeed */
    domain.spawn(2);
    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter3;
    domain.sendGetStatusEpSvcRequest(1, 2, svcStatusWaiter3);
    svcStatusWaiter3.await();
    ASSERT_EQ(svcStatusWaiter3.error, ERR_OK)
        << "Error: " << svcStatusWaiter3.error;
    ASSERT_EQ(svcStatusWaiter3.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter3.response->status;
}

/**
* @brief Test sending endpoint request agains invalid endpoint
*
*/
TEST_F(SvcRequestMgrTest, epsvcrequest_invalidep)
{
    int cnt = 3;
    FakeSyncSvcDomain domain(cnt, confFile);

    auto svcMgr1 = domain[1]->getSvcMgr();

    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = svcMgr1->getSvcRequestMgr()->newEPSvcRequest(FakeSvcFactory::INVALID_SVCUUID);
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onResponseCb(svcStatusWaiter.cb);
    asyncReq->invoke();

    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_SVC_REQUEST_INVOCATION) << "Error: " << svcStatusWaiter.error;
}


/* Tests basic failover style request */
TEST_F(SvcRequestMgrTest, failoversvcrequest)
{
    int cnt = 4;
    FakeSyncSvcDomain domain(cnt, confFile);

    /* all endpoints are up...request should succeed */
    SvcRequestCbTask<FailoverSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    domain.sendGetStatusFailoverSvcRequest(1, {2,3}, svcStatusWaiter);

    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_OK) << "Error: " << svcStatusWaiter.error;
    ASSERT_EQ(svcStatusWaiter.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter.response->status;

    /* one endpoints is down...request should succeed */
    domain.kill(2);
    SvcRequestCbTask<FailoverSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter1;
    domain.sendGetStatusFailoverSvcRequest(1, {2,3}, svcStatusWaiter1);
    svcStatusWaiter1.await();
    ASSERT_EQ(svcStatusWaiter1.error, ERR_OK) << "Error: " << svcStatusWaiter1.error;
    ASSERT_EQ(svcStatusWaiter1.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter1.response->status;

    /* all endpoints are down. Request should fail */
    domain.kill(3);
    SvcRequestCbTask<FailoverSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter2;
    domain.sendGetStatusFailoverSvcRequest(1, {2,3}, svcStatusWaiter2);
    svcStatusWaiter2.await();
    ASSERT_TRUE(svcStatusWaiter2.error == ERR_SVC_REQUEST_INVOCATION ||
                svcStatusWaiter2.error == ERR_SVC_REQUEST_TIMEOUT)
        << "Error: " << svcStatusWaiter2.error;

    /* One service is up. Request should succeed */
    domain.spawn(3);
    SvcRequestCbTask<FailoverSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter3;
    domain.sendGetStatusFailoverSvcRequest(1, {2,3}, svcStatusWaiter3);
    svcStatusWaiter3.await();
    ASSERT_EQ(svcStatusWaiter3.error, ERR_OK) << "Error: " << svcStatusWaiter3.error;
    ASSERT_EQ(svcStatusWaiter3.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter3.response->status;
}

/* Tests basic quorum reqesut */
TEST_F(SvcRequestMgrTest, quorumsvcrequest)
{
    int cnt = 4;
    FakeSyncSvcDomain domain(cnt, confFile);

    SvcRequestCbTask<QuorumSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter;
    domain.sendGetStatusQuorumSvcRequest(1, {2,3}, svcStatusWaiter);

    svcStatusWaiter.await();
    ASSERT_EQ(svcStatusWaiter.error, ERR_OK) << "Error: " << svcStatusWaiter.error;
    ASSERT_EQ(svcStatusWaiter.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter.response->status;

    /* Kill both services.  Quorum request should fail */
    domain.kill(2);
    domain.kill(3);
    SvcRequestCbTask<QuorumSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter1;
    domain.sendGetStatusQuorumSvcRequest(1, {2,3}, svcStatusWaiter1);
    svcStatusWaiter1.await();
    ASSERT_TRUE(svcStatusWaiter1.error == ERR_SVC_REQUEST_INVOCATION ||
                svcStatusWaiter1.error == ERR_SVC_REQUEST_TIMEOUT)
        << "Error: " << svcStatusWaiter1.error;

    /* Bring one service up.  Quorum request should fail */
    domain.spawn(2);
    SvcRequestCbTask<QuorumSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter2;
    domain.sendGetStatusQuorumSvcRequest(1, {2,3}, svcStatusWaiter2);
    svcStatusWaiter2.await();
    ASSERT_TRUE(svcStatusWaiter2.error == ERR_SVC_REQUEST_INVOCATION ||
                svcStatusWaiter2.error == ERR_SVC_REQUEST_TIMEOUT)
        << "Error: " << svcStatusWaiter2.error;

    /* Bring both services up.  Quorum request should succeed */
    domain.spawn(3);
    SvcRequestCbTask<QuorumSvcRequest, fpi::GetSvcStatusRespMsg> svcStatusWaiter3;
    domain.sendGetStatusQuorumSvcRequest(1, {2,3}, svcStatusWaiter3);
    svcStatusWaiter3.await();
    ASSERT_EQ(svcStatusWaiter3.error, ERR_OK) << "Error: " << svcStatusWaiter3.error;
    ASSERT_EQ(svcStatusWaiter3.response->status, fpi::SVC_STATUS_ACTIVE)
        << "Status: " << svcStatusWaiter3.response->status;
}

TEST_F(SvcRequestMgrTest, multiPrimarySvcRequest) {
    int cnt = 5;
    FakeSyncSvcDomain domain(cnt, confFile);
    MultiPrimarySvcRequestPtr req;
    /* Request with all services up should work */
    MultiPrimarySvcRequestCbTask svcStatusWaiter1;
    req = domain.sendGetStatusMultiPrimarySvcRequest(
        1,
        {2,3}, {4},
        svcStatusWaiter1);
    svcStatusWaiter1.await();
    ASSERT_EQ(svcStatusWaiter1.error, ERR_OK) << "Error: " << svcStatusWaiter1.error;
    ASSERT_EQ(req->getFailedPrimaries().size(), 0);
    ASSERT_EQ(req->getFailedOptionals().size(), 0);

    /* Request with optional service down should work */
    domain.kill(4);
    MultiPrimarySvcRequestCbTask svcStatusWaiter2;
    req = domain.sendGetStatusMultiPrimarySvcRequest(
        1,
        {2,3}, {4},
        svcStatusWaiter2);
    svcStatusWaiter2.await();
    ASSERT_EQ(svcStatusWaiter2.error, ERR_OK) << "Error: " << svcStatusWaiter2.error;
    ASSERT_EQ(req->getFailedPrimaries().size(), 0);
    ASSERT_EQ(req->getFailedOptionals().size(), 1);

    /* Request with one primary down should fail */
    domain.spawn(4);
    domain.kill(2);
    MultiPrimarySvcRequestCbTask svcStatusWaiter3;
    req = domain.sendGetStatusMultiPrimarySvcRequest(
        1,
        {2,3}, {4},
        svcStatusWaiter3);
    svcStatusWaiter3.await();
    ASSERT_EQ(svcStatusWaiter3.error, ERR_SVC_REQUEST_TIMEOUT)
        << "Error: " << svcStatusWaiter3.error;
    ASSERT_EQ(req->getFailedPrimaries().size(), 1);
    ASSERT_EQ(req->getFailedOptionals().size(), 0);
}

struct FTCallback : concurrency::TaskStatus {
    void handle(fds::net::FileTransferService::Handle::ptr handle,
                const Error &e) {
        GLOGNORMAL << "in callback : " << handle->srcFile << " : " << e;
        done();
    }
};

TEST_F(SvcRequestMgrTest, filetransfer) {
    int cnt = 3;
    FakeSyncSvcDomain domain(cnt, confFile);
    FTCallback cb;
    fds::net::FileTransferService::OnTransferCallback ftcb = std::bind(&FTCallback::handle, &cb, std::placeholders::_1, std::placeholders::_2);
    domain[1]->filetransfer->send(domain.getFakeSvcUuid(2), "/bin/ls", "test.txt", ftcb, false);
    cb.await();
    cb.reset(1);
    domain[1]->filetransfer->send(domain.getFakeSvcUuid(2), "/bin/ls", "test.txt", ftcb, false);
    cb.await();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("fds-root", po::value<std::string>()->default_value("/fds"), "root");
    g_fdslog = new fds_log("SvcRequestMgrTest");
    SvcRequestMgrTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

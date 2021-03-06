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
#include <util/stats.h>
#include <util/memutils.h>
#include <util/path.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

struct SvcRequestMgrTest : BaseTestFixture {
    static std::string fdsRoot;

    static void SetUpTestCase() {
        fdsRoot = getArg<std::string>("fds-root");
    }

};
std::string SvcRequestMgrTest::fdsRoot;

/**
* @brief Tests svc map update in the domain.
*/
TEST_F(SvcRequestMgrTest, epsvcrequest)
{
    int cnt = 3;
    FakeSyncSvcDomain domain(cnt, fdsRoot);

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
    FakeSyncSvcDomain domain(cnt, fdsRoot);

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
    FakeSyncSvcDomain domain(cnt, fdsRoot);

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
    FakeSyncSvcDomain domain(cnt, fdsRoot);

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
    FakeSyncSvcDomain domain(cnt, fdsRoot);
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
        GLOGNORMAL << "in callback src: " << handle->srcFile << " dest:" << handle->destFile << " : " << e;
        error = e;
        done();
    }
};

TEST_F(SvcRequestMgrTest, filetransfer) {
    int cnt = 3;
    FakeSyncSvcDomain domain(cnt, fdsRoot);
    FTCallback cb;
    fds::net::FileTransferService::OnTransferCallback ftcb = std::bind(&FTCallback::handle, &cb, std::placeholders::_1, std::placeholders::_2);
    util::Stats stats;
    auto bytes = util::getMemoryKB();
    ulong ftTimeout = 30*1000; // in millis
    bytes = util::getMemoryKB();
    std::cout << "mem:" << bytes << " : init" << std::endl;
    domain[1]->filetransfer->setChunkSize(1024);
    domain[1]->filetransfer->send(domain.getFakeSvcUuid(2), "/bin/ls", "ls-test", ftcb, false);
    cb.await(ftTimeout);
    ASSERT_EQ(ERR_OK, cb.error);
    cb.reset(1); cb.error = ERR_PENDING_RESP;

    bytes = util::getMemoryKB();
    std::cout << "mem:" << bytes << " : after 1 file - small chunk " << std::endl;
    domain[1]->filetransfer->setChunkSize(5*KB);
    stats.reset();
    stats.add(bytes);
    for (uint i = 0 ; i < 100 ; i++) {
        domain[1]->filetransfer->send(domain.getFakeSvcUuid(2), "/bin/ls", "test.txt_" + std::to_string(i), ftcb, false);
        cb.await(ftTimeout);
        ASSERT_EQ(ERR_OK, cb.error);
        cb.reset(1); cb.error = ERR_PENDING_RESP;
        usleep(1000);
        stats.add(util::getMemoryKB());
    }
    stats.calculate();
    bytes = util::getMemoryKB();
    auto originalBytes = bytes;
    std::cout << "mem:" << bytes << " : after 100 /bin/ls xfers : " << stats <<std::endl;

    domain[1]->filetransfer->setChunkSize(2*MB);
    stats.reset();
    stats.add(bytes);
    for (uint i = 0 ; i < 100 ; i++) {
        domain[1]->filetransfer->send(domain.getFakeSvcUuid(2), "/bin/ls", "test.txt_" + std::to_string(i), ftcb, false);
        cb.await(ftTimeout);
        ASSERT_EQ(ERR_OK, cb.error);
        cb.reset(1); cb.error = ERR_PENDING_RESP;
        usleep(1000);
        stats.add(util::getMemoryKB());
    }
    stats.calculate();

    bytes = util::getMemoryKB();
    std::cout << "mem:" << bytes << " : after 100 /bin/ls re-xfers : " << stats << std::endl;


    std::cout << "original:" << originalBytes << " now:" << bytes << " diff:" << (bytes-originalBytes) << std::endl;
    auto diffBytes = bytes-originalBytes;
    if (diffBytes > 8*KB) {
        GLOGWARN << "memory usage seems to have increased unusually : " << (diffBytes/KB) << " KB";
    }
    EXPECT_LT(bytes-originalBytes, 15*KB);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    std::string root;
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("fds-root", po::value<std::string>(&root)->default_value("/fds"), "root");
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(opts)
              .allow_unregistered()
              .run(), vm);
    po::notify(vm);
    FdsRootDir fdsroot(root+"/");
    g_fdslog = new fds_log(fdsroot.dir_fds_logs() + "/SvcRequestMgrTest", fdsroot.dir_fds_logs());
    SvcRequestMgrTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

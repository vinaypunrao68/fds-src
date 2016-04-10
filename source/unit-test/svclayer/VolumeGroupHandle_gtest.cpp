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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testlib/TestFixtures.h>
#include <testlib/TestUtils.h>
#include <net/VolumeGroupHandle.h>
#include <testlib/FakeSvcDomain.hpp>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

struct VolumeCoordinatorTest : BaseTestFixture {
    void setDmHandlers(const PlatNetSvcHandlerPtr &h) {
        h->updateHandler(
            FDSP_MSG_TYPEID(fpi::OpenVolumeMsg),
            [h](fpi::AsyncHdrPtr &hdr, StringPtr &payload) {
                fpi::OpenVolumeRspMsg resp;
                resp.replicaVersion = VolumeGroupConstants::VERSION_START;
                h->sendAsyncResp(*hdr,
                                 FDSP_MSG_TYPEID(fpi::OpenVolumeRspMsg),
                                 resp);
            });
        h->updateHandler(
            FDSP_MSG_TYPEID(fpi::StatVolumeMsg),
            [h](fpi::AsyncHdrPtr &hdr, StringPtr &payload) {
                h->sendAsyncResp(*hdr,
                                 FDSP_MSG_TYPEID(fpi::StatVolumeRspMsg),
                                 fpi::StatVolumeRspMsg());
            });
        h->updateHandler(
            FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg),
            [h](fpi::AsyncHdrPtr &hdr, StringPtr &payload) {
                h->sendAsyncResp(*hdr,
                                 FDSP_MSG_TYPEID(fpi::UpdateCatalogRspMsg),
                                 fpi::UpdateCatalogRspMsg());
            });
        h->updateHandler(
            FDSP_MSG_TYPEID(fpi::CommitBlobTxMsg),
            [h](fpi::AsyncHdrPtr &hdr, StringPtr &payload) {
                h->sendAsyncResp(*hdr,
                                 FDSP_MSG_TYPEID(fpi::CommitBlobTxRspMsg),
                                 fpi::CommitBlobTxRspMsg());
            });
    }
    void setOmHandlers(const PlatNetSvcHandlerPtr &h) {
        h->updateHandler(
            FDSP_MSG_TYPEID(fpi::SetVolumeGroupCoordinatorMsg),
            [h](fpi::AsyncHdrPtr &hdr, StringPtr &payload) {
                static int version = 0;
                auto resp = MAKE_SHARED<fpi::SetVolumeGroupCoordinatorRspMsg>();
                resp->version = ++version;
                h->sendAsyncResp(*hdr,
                                 FDSP_MSG_TYPEID(fpi::SetVolumeGroupCoordinatorRspMsg),
                                 *resp);
            });
    }
    void setAmHandlers(const PlatNetSvcHandlerPtr &h) {
    }
};

/**
* @brief Tests svc map update in the domain.
*/
TEST_F(VolumeCoordinatorTest, basicio) {
    concurrency::TaskStatus waiter;
    int cnt = 5;
    FakeSyncSvcDomain domain(cnt, this->getArg<std::string>("fds-root") +
                             std::string("/etc/platform.conf"));
    auto dmt = DMT::newDMT({
                           domain[2]->getSvcMgr()->getSelfSvcUuid(),
                           domain[3]->getSvcMgr()->getSelfSvcUuid(),
                           domain[4]->getSvcMgr()->getSelfSvcUuid()
                           });
    auto group = dmt->getNodeGroup(fds_volid_t(10));
    ASSERT_TRUE(group->getLength() == 3);

    domain.updateDmt(dmt);
    setOmHandlers(domain[0]->getSvcMgr()->getSvcRequestHandler());
    setOmHandlers(domain[1]->getSvcMgr()->getSvcRequestHandler());
    setDmHandlers(domain[2]->getSvcMgr()->getSvcRequestHandler());
    setDmHandlers(domain[3]->getSvcMgr()->getSvcRequestHandler());
    setDmHandlers(domain[4]->getSvcMgr()->getSvcRequestHandler());

    /* Create a group with quorum of 2 out of 3 */
    fds_volid_t v1Id(10);
    VolumeGroupHandle v1(domain[1], v1Id, 2);

    /* Open the group */
    v1.open(MAKE_SHARED<fpi::OpenVolumeMsg>(),
            [&waiter](const Error &e, const fpi::OpenVolumeRspMsgPtr&) {
                ASSERT_TRUE(e == ERR_OK);
                waiter.done();
            });
    waiter.await();

    /* We will use this message for all requests below..Although it doesn't make sense
     * from DM semantics, for testing out coordinator this fine
     */
    auto statMsg = MAKE_SHARED<fpi::StatVolumeMsg>();
    auto updateMsg = MAKE_SHARED<fpi::UpdateCatalogMsg>();
    auto commitMsg = MAKE_SHARED<fpi::CommitBlobTxMsg>();

    /* Do a modify request */
    waiter.reset(1);
    v1.sendModifyMsg<fpi::UpdateCatalogMsg>(
        FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg),
        updateMsg,
        [&waiter](const Error &e, StringPtr) {
            ASSERT_TRUE(e == ERR_OK);
            waiter.done();
        });
    waiter.await();

    /* Do a write request */
    waiter.reset(1);
    v1.sendCommitMsg<fpi::CommitBlobTxMsg>(
        FDSP_MSG_TYPEID(fpi::CommitBlobTxMsg),
        commitMsg,
        [&waiter](const Error &e, StringPtr) {
            ASSERT_TRUE(e == ERR_OK);
            waiter.done();
        });
    waiter.await();

    /* Do a read request */
    waiter.reset(1);
    v1.sendReadMsg<fpi::StatVolumeMsg>(
        FDSP_MSG_TYPEID(fpi::StatVolumeMsg),
        statMsg,
        [&waiter](const Error &e, StringPtr) {
            ASSERT_TRUE(e == ERR_OK);
            waiter.done();
        });
    waiter.await();

    /* Fail one service */
    domain.kill(2);
    LOGNORMAL << "Killed servic #2";

    /* Modify should succeed */
    waiter.reset(1);
    v1.sendModifyMsg<fpi::UpdateCatalogMsg>(
        FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg),
        updateMsg,
        [&waiter](const Error &e, StringPtr) {
            LOGNOTIFY << "Cb called";
            ASSERT_TRUE(e == ERR_OK);
            waiter.done();
        });
    waiter.await();
    LOGNOTIFY << "Wait finished";
    POLL_MS(v1.getFunctionalReplicasCnt() == 2, 200, 7000);
    ASSERT_TRUE(v1.getFunctionalReplicasCnt() == 2);

    /* Read should succeed */
    waiter.reset(1);
    v1.sendReadMsg<fpi::StatVolumeMsg>(
        FDSP_MSG_TYPEID(fpi::StatVolumeMsg),
        statMsg ,
        [&waiter](const Error &e, StringPtr) {
            ASSERT_TRUE(e == ERR_OK);
            waiter.done();
        });
    waiter.await();
    ASSERT_TRUE(v1.getFunctionalReplicasCnt() == 2);

    /* Fail another service */
    domain.kill(3);
    LOGNORMAL << "Killed servic #3";

    /* Modify should fail */
    waiter.reset(1);
    v1.sendModifyMsg<fpi::UpdateCatalogMsg>(
        FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg),
        updateMsg,
        [&waiter](const Error &e, StringPtr) {
            ASSERT_TRUE(e != ERR_OK);
            waiter.done();
        });
    waiter.await();
    ASSERT_TRUE(v1.getFunctionalReplicasCnt() == 0);
    ASSERT_TRUE(v1.isFunctional() == false);

    /* Read should fail */
    waiter.reset(1);
    v1.sendReadMsg<fpi::StatVolumeMsg>(
        FDSP_MSG_TYPEID(fpi::StatVolumeMsg),
        statMsg ,
        [&waiter](const Error &e, StringPtr) {
            ASSERT_TRUE(e != ERR_OK);
            waiter.done();
        });
    waiter.await();
    ASSERT_TRUE(v1.getFunctionalReplicasCnt() == 0);
    ASSERT_TRUE(v1.isFunctional() == false);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("fds-root", po::value<std::string>()->default_value("/fds"), "root");
    g_fdslog = new fds_log("VolumeGrouping");
    VolumeCoordinatorTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

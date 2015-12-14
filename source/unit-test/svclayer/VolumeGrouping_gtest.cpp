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
#include <FakeSvcDomain.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testlib/TestFixtures.h>
#include <net/VolumeGroupHandle.h>
#include <FakeSvcDomain.hpp>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

struct VolumegroupingTest : BaseTestFixture {
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
            FDSP_MSG_TYPEID(fpi::StatVolumeMsg),
            [h](fpi::AsyncHdrPtr &hdr, StringPtr &payload) {
                h->sendAsyncResp(*hdr,
                                 FDSP_MSG_TYPEID(fpi::StatVolumeRspMsg),
                                 fpi::StatVolumeRspMsg());
            });
    }
    void setOmHandlers(const PlatNetSvcHandlerPtr &h) {
        h->updateHandler(
            FDSP_MSG_TYPEID(fpi::SetVolumeGroupCoordinatorMsg),
            [h](fpi::AsyncHdrPtr &hdr, StringPtr &payload) {
                h->sendAsyncResp(*hdr,
                                 FDSP_MSG_TYPEID(fpi::EmptyMsg),
                                 fpi::EmptyMsg());
            });
    }
    void setAmHandlers(const PlatNetSvcHandlerPtr &h) {
    }
};

/**
* @brief Tests svc map update in the domain.
*/
TEST_F(VolumegroupingTest, basicio) {
    concurrency::TaskStatus waiter;
    int cnt = 4;
    FakeSyncSvcDomain domain(cnt, this->getArg<std::string>("fds-root") +
                             std::string("/etc/platform.conf"));
    auto dmt = DMT::newDMT({
                           domain[2]->getSvcMgr()->getSelfSvcUuid(),
                           domain[3]->getSvcMgr()->getSelfSvcUuid()
                           });
    auto group = dmt->getNodeGroup(fds_volid_t(10));
    ASSERT_TRUE(group->getLength() == 2);

    domain.updateDmt(dmt);
    setOmHandlers(domain[0]->getSvcMgr()->getSvcRequestHandler());
    setOmHandlers(domain[1]->getSvcMgr()->getSvcRequestHandler());
    setDmHandlers(domain[2]->getSvcMgr()->getSvcRequestHandler());
    setDmHandlers(domain[3]->getSvcMgr()->getSvcRequestHandler());

    /* Basic opening should succeed */
    fds_volid_t v1Id(10);
    VolumeGroupHandle v1(domain[1], v1Id, 2);
    v1.open(MAKE_SHARED<fpi::OpenVolumeMsg>(),
            [&waiter](const Error &e) {
                ASSERT_TRUE(e == ERR_OK);
                waiter.done();
            });
    waiter.await();

    waiter.reset(1);
    auto statMsg = MAKE_SHARED<fpi::StatVolumeMsg>();
    v1.sendReadMsg<fpi::StatVolumeMsg>(
        FDSP_MSG_TYPEID(fpi::StatVolumeMsg),
        statMsg ,
        [&waiter](const Error &e, StringPtr) {
            ASSERT_TRUE(e == ERR_OK);
            waiter.done();
        });
    waiter.await();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("fds-root", po::value<std::string>()->default_value("/fds"), "root");
    g_fdslog = new fds_log("VolumeGrouping");
    VolumegroupingTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

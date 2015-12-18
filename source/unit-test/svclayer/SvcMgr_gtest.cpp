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

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

struct SvcMgrTest : BaseTestFixture {
};

/**
* @brief Tests svc map update in the domain.
*/
TEST_F(SvcMgrTest, svcmapUdpate) {
    int cnt = 5;
    FakeSyncSvcDomain domain(cnt, this->getArg<std::string>("fds-root") +
                             std::string("/etc/platform.conf"));
    for (int svcIdx = 1; svcIdx < cnt; svcIdx++) {
        ASSERT_TRUE(domain.checkSvcInfoAgainstDomain(svcIdx));
    }

    for (int svcIdx = 1; svcIdx < cnt; svcIdx++) {
        domain.kill(svcIdx);
        domain.spawn(svcIdx);
        ASSERT_TRUE(domain.checkSvcInfoAgainstDomain(svcIdx));
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("fds-root", po::value<std::string>()->default_value("/fds"), "root");
    g_fdslog = new fds_log("SvcMgrTest");
    SvcMgrTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

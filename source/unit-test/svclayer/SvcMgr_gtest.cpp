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

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

/**
* @brief Tests svc map update in the domain.
*/
TEST(SvcMgr, svcmapUdpate) {
    int cnt = 4;
    FakeSyncSvcDomain domain(cnt);
    for (int svcIdx = 0; svcIdx < cnt; svcIdx++) {
        ASSERT_TRUE(domain.checkSvcInfoAgainstDomain(svcIdx));
    }

    for (int svcIdx = 0; svcIdx < cnt; svcIdx++) {
        domain.kill(svcIdx);
        domain.spawn(svcIdx);
        ASSERT_TRUE(domain.checkSvcInfoAgainstDomain(svcIdx));
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

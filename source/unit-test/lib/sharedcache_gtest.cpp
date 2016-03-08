/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <cstdio>
#include <string>
#include <vector>
#include <bitset>
#include "boost/smart_ptr/make_shared.hpp"

#include <fds_types.h>
#include <cache/SharedKvCache.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

TEST(SharedKvCache, add_get)
{
    SharedKvCache<fds_uint32_t, fds_uint32_t, std::hash<fds_uint32_t>>
        cacheManager("Integer cache manager", 50);

    fds_uint32_t k1 = 1;
    fds_uint32_t k2 = 2;
    fds_uint32_t k3 = 3;
    fds_uint32_t k4 = 4;
    cacheManager.add(k1, k1);

    cacheManager.add(k2, k2);
    cacheManager.add(k3, k3);
    cacheManager.add(k4, k4);

    // Test get works
    decltype(cacheManager)::value_type getV2;
    fds::Error err = cacheManager.get(k2, getV2);
    EXPECT_TRUE(err == fds::ERR_OK);
    EXPECT_TRUE(*getV2 == k2);

    // Test overwrite
    cacheManager.add(k2, k3);
    err = cacheManager.get(k2, getV2);
    EXPECT_TRUE(err == fds::ERR_OK);
    EXPECT_TRUE(*getV2 == k3);
}

TEST(SharedKvCache, add_get_strong)
{
    SharedKvCache<fds_uint32_t, fds_uint32_t, std::hash<fds_uint32_t>, std::true_type>
        cacheManager("Integer cache manager", 50);

    fds_uint32_t k1 = 1;
    fds_uint32_t k2 = 2;
    fds_uint32_t k3 = 3;
    fds_uint32_t k4 = 4;
    cacheManager.add(k1, k1);

    cacheManager.add(k2, k2);
    cacheManager.add(k3, k3);
    cacheManager.add(k4, k4);

    // Test get works
    decltype(cacheManager)::value_type getV2;
    fds::Error err = cacheManager.get(k2, getV2);
    EXPECT_TRUE(err == fds::ERR_OK);
    EXPECT_TRUE(*getV2 == k2);

    // Test overwrite
    cacheManager.add(k2, k3);
    err = cacheManager.get(k2, getV2);
    EXPECT_TRUE(err == fds::ERR_OK);
    EXPECT_FALSE(*getV2 == k3);
    EXPECT_TRUE(*getV2 == k2);
}

TEST(SharedKvCache, eviction)
{
    uint32_t cacheSz = 20;
    SharedKvCache<fds_uint32_t, fds_uint32_t> cacheManager("Integer cache manager", cacheSz);

    // Until we reach cache size limit, we shouldn't have evictions
    uint32_t i;
    for (i = 0; i < cacheSz; i++) {
        auto evicted = cacheManager.add(i, i);
        EXPECT_FALSE(evicted);
    }
    // We've reached size limit.  We should have an eviction
    auto evicted = cacheManager.add(i, i);
    EXPECT_TRUE(evicted);
    // Cache size should be at the limit now
    EXPECT_TRUE(cacheManager.getSize() == cacheSz);
}

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}


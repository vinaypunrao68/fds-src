
/*
 *  Copyright 2016 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <util/stringutils.h>
#include <concurrency/RwLock.h>
#include <testlib/VolumeGroupFixture.hpp>
#include <testlib/ProcessHandle.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <net/VolumeGroupHandle.h>
#include <boost/filesystem.hpp>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;
using namespace fds::TestUtils;

/**
 * Test 1: Positive test
 * 1. Create a VG of 2 DMs with a single volume.
 * 2. Write IO to both DMs on the volume.
 * 3. Run Volume checker on volume, should return consistent result.
 */
TEST_F(VolumeGroupFixture, twoHappyDMs) {
    LOGNORMAL << "TEST MARKER Starting twoHappyDMs";

    // Create 2 DMs
    unsigned clusterSize = 2;
    createCluster(clusterSize);
    setupVolumeGroup(clusterSize);

    // Test Volume Checker svcmgr layer
    // For now only one volume
    std::vector<unsigned> volIdList(1);
    initVolumeChecker(volIdList);
    addDMTToVC(getOmDMT(DMT_COMMITTED));
    stopVolumeChecker();

    /* Do some io. After Io is done, every volume replica must have same state */
#if 0
    for (uint32_t i = 0; i < 10; i++) {
        sendUpdateOnceMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        doGroupStateCheck(v1Id);
    }
#endif


    LOGNORMAL << "TEST MARKER Finished twoHappyDMs";
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    g_fdslog = new fds_log("volume_checker_gtest");
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>()->default_value(1), "puts count");
    VolumeGroupFixture::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

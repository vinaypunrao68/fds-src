
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
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);
    LOGNORMAL << "TEST MARKER Starting twoHappyDMs";

    // Create 2 DMs
    unsigned clusterSize = 2;
    createCluster(clusterSize);
    setupVolumeGroup(clusterSize);

    // Test Volume Checker svcmgr layer
    // For now only one volume
    std::vector<unsigned> volIdList;
    volIdList.push_back(v1Id.v);

    /* Do some io. After Io is done, every volume replica must have same state */
    for (uint32_t i = 0; i < 10; i++) {
        sendUpdateOnceMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        doGroupStateCheck(v1Id);
    }

    // Volume checker run should be consistent
    runVolumeChecker(volIdList, clusterSize);
    stopVolumeChecker();

    LOGNORMAL << "TEST MARKER Finished twoHappyDMs";
}

/**
 * Test 2: Test resync
 * 1. Create a VG of 3 DMs with a single volume
 * 2. Write IOs
 * 3. Bring down a DM
 * 4. Write more IOs
 * 5. Bring up a DM
 * 6. Write IOs for 10 second to wait for resync
 * 7. Test VC - should pass
 */
TEST_F(VolumeGroupFixture, staticio_restarts_with_vc) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    /* Create two dms */
    unsigned clusterSize = 3;
    createCluster(clusterSize);
    setupVolumeGroup(1);

    /* Do some io. After Io is done, every volume replica must have same state */
    for (uint32_t i = 0; i < 10; i++) {
        sendUpdateOnceMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        doGroupStateCheck(v1Id);
    }

    ASSERT_TRUE(dmGroup[0]->proc->getDataMgr()->counters->totalVolumesReceivedMigration.value() == 0);
    /* Bring a dm down */
    dmGroup[0]->stop();
    /* Do more IO.  IO should succeed */
    for (uint32_t i = 0; i < 10; i++) {
        blobName = "blob" + std::to_string(i);
        sendUpdateOnceMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    }
    /* Send few more non-commit updates so that we have active Txs */
    for (uint32_t i = 0; i < 5; i++) {
        sendStartBlobTxMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    }

    /* Bring 1st dm up again */
    dmGroup[0]->start();

    /* Keep doing IO for maximum of 10 seconds */
    for (uint32_t i = 0;
         (i < 100 &&
          dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() != fpi::Active);
         i++) {
        sendUpdateOnceMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(*v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        if (i % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    /* By now sync must complete */
    doVolumeStateCheck(0, v1Id, fpi::Active);
    doMigrationCheck(0, v1Id);

    /* Do more IO.  IO should succeed */
    doIo(10);

    // For now only one volume
    std::vector<unsigned> volIdList;
    volIdList.push_back(v1Id.v);

    // Volume checker run should be consistent
    runVolumeChecker(volIdList, clusterSize);
    stopVolumeChecker();

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
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

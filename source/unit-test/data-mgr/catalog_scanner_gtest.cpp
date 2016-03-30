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
#include <CatalogScanner.h>
#include <leveldb/db.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;
using namespace fds::TestUtils;

TEST_F(VolumeGroupFixture, scannerInitialization) {
    // Create 1 DM
    unsigned clusterSize = 1;
    createCluster(1);
    setupVolumeGroupHandleOnAm1(1);

    auto catalogPtr = dmGroup[0]->proc->getDataMgr()->getPersistDB(v1Id)->getCatalog();
    auto threadpoolPtr = dmGroup[0]->proc->getDataMgr()->qosCtrl->threadPool;

    std::list<CatalogKVPair> batchSlice;
    CatalogScanner::progress scannerProgress;

    auto batchCb = [&batchSlice](std::list<CatalogKVPair> &batchSlice) {
        // TODO
    };

    auto scannerCb = [this](CatalogScanner::progress scannerProgress) {
        // TODO
    };
    CatalogScanner scanner(*catalogPtr,
                           threadpoolPtr,
                           10,
                           batchCb,
                           scannerCb);

    ASSERT_TRUE(scanner.getProgress() == CatalogScanner::progress::CS_INIT);

    scanner.start();
    sleep(1);

    ASSERT_TRUE(scanner.getProgress() == CatalogScanner::progress::CS_DONE);

}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    g_fdslog = new fds_log("catalog_scanner_gtest");
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>()->default_value(1), "puts count");
    VolumeGroupFixture::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

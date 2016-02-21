
/* Copyright 2015 Formation Data Systems, Inc.
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
#include <net/VolumeGroupHandle.h>
#include <boost/filesystem.hpp>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;
using namespace fds::TestUtils;


TEST_F(VolumeGroupFixture, singledm) {
    /* Start with one dm */
    createCluster(1);

    /* Create a DM group */
    /* Create a coordinator */
    v1 = MAKE_SHARED<VolumeGroupHandle>(amHandle.proc, v1Id, 1);
    amHandle.proc->setVolumeHandle(v1.get());

    /* Open the group without DMT.  Open should fail */
    openVolume(*v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);

    /* Add DMT */
    addDMT();

    /* Open without volume being add to DM.  Open should fail */
    openVolume(*v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOL_NOT_FOUND);

    /* Generate volume descriptor */
    v1Desc = generateVolume(v1Id);
    omHandle.proc->addVolume(v1Id, v1Desc);

    /* Add volume to DM */
    Error e = dmGroup[0]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
    ASSERT_TRUE(e == ERR_OK);
    ASSERT_TRUE(dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Offline);
    /* Any IO should fail */
    sendUpdateOnceMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOLUMEGROUP_DOWN);

    /* Now open should succeed */
    openVolume(*v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Do some io. After Io is done, every volume replica must have same state */
    sendUpdateOnceMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    ASSERT_TRUE(dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Active);
    sendQueryCatalogMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Bring a dm down */
    dmGroup[0]->stop();
    /* Do more IO.  IO should fail */
    sendQueryCatalogMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(VolumeGroupFixture, staticio_restarts) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::trace);

    /* Create two dms */
    createCluster(2);
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

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(VolumeGroupFixture, activeio_restart) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);
    /* Create two dms */
    createCluster(2);
    setupVolumeGroup(1);

    /* Start io thread */
    bool ioAbort = false;
    std::thread iothread([this, &ioAbort]() {
        int ioCnt = 0;
        while (!ioAbort) {
            sendUpdateOnceMsg(*v1, blobName, waiter);
            ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
            sendQueryCatalogMsg(*v1, blobName, waiter);
            ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
            ioCnt++;
            if (ioCnt % 10) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });

    ASSERT_TRUE(dmGroup[0]->proc->getDataMgr()->counters->totalVolumesReceivedMigration.value() == 0);
    for (int i = 0; i < 4; i++) {
        /* Bring a dm down */
        dmGroup[0]->stop();

        /* Bring 1st dm up again */
        dmGroup[0]->start();
        doVolumeStateCheck(0, v1Id, fpi::Active);
        doMigrationCheck(0, v1Id);
    }

    ioAbort = true;
    iothread.join();

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(VolumeGroupFixture, domain_reboot) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    /* Create two dms */
    createCluster(2);
    setupVolumeGroup(1);

    /* Do more IO.  IO should succeed */
    doIo(10);

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Stop all the DMs */
    for (uint32_t i = 0; i < dmGroup.size(); i++) {
        dmGroup[i]->stop();
    }

    /* Start all dms */
    for (uint32_t i = 0; i < dmGroup.size(); i++) {
        dmGroup[i]->start();
    }

    /* Open the handle */
    openVolume(*v1, waiter);
    waiter.awaitResult();
    POLL_MS(v1->getFunctionalReplicasCnt() == dmGroup.size(), 1000, 8000);
    ASSERT_TRUE(v1->getFunctionalReplicasCnt() == dmGroup.size());


    /* Do more IO.  IO should succeed */
    doIo(10);

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(VolumeGroupFixture, allDownFollowedBySequentialUp) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    /* Create two dms */
    createCluster(3);
    setupVolumeGroup(2);

    /* Do more IO.  IO should succeed */
    doIo(10);

    /* stop two dms with out active.  So coordinator still thinks all dms are active */
    dmGroup[0]->stop();
    dmGroup[1]->stop();
    dmGroup[2]->stop();
    LOGNORMAL << "Stopped all the dms";

    /* Bring up one dm at a time */
    dmGroup[2]->start();
    LOGNORMAL << "Started dm2";


    dmGroup[1]->start();
    LOGNORMAL << "Started dm1";

    dmGroup[0]->start();
    LOGNORMAL << "Started dm0";

    doVolumeStateCheck(0, v1Id, fpi::Active);
    doVolumeStateCheck(1, v1Id, fpi::Active, 1000, 10000);
    doVolumeStateCheck(2, v1Id, fpi::Active, 1000, 10000);
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    g_fdslog = new fds_log("volumegtest");
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>()->default_value(1), "puts count");
    VolumeGroupFixture::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

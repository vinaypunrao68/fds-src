
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

#define MAX_OBJECT_SIZE 1024 * 1024 * 2

TEST_F(VolumeGroupFixture, singledm) {
    /* Start with one dm */
    unsigned clusterSize=1;
    createCluster(clusterSize);

    /* Create a DM group */
    /* Create a coordinator */
    v1 = MAKE_SHARED<VolumeGroupHandle>(amHandle.proc, v1Id, 1);
    amHandle.proc->setVolumeHandle(v1.get());

    /* Open the group without DMT.  Open should fail */
    openVolume(*v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);

    /* Add DMT */
    std::string dmtData;
    std::vector<fpi::SvcUuid> dmtColumn;
    for (unsigned i = 0; i < clusterSize; i++) {
        dmtColumn.emplace_back(dmGroup[i]->proc->getSvcMgr()->getSelfSvcUuid());
    }
    dmt = DMT::newDMT(dmtColumn);
    dmt->getSerialized(dmtData);
    Error e = amHandle.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                            nullptr,
                                                                            DMT_COMMITTED);
    ASSERT_TRUE(e == ERR_OK);
    for (unsigned i = 0; i < clusterSize; i++) {
        e = dmGroup[i]->proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                             nullptr,
                                                                             DMT_COMMITTED);
    }
    ASSERT_TRUE(e == ERR_OK);

    /* Open without volume being add to DM.  Open should fail */
    openVolume(*v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOLUMEGROUP_INVALID);

    /* Generate volume descriptor */
    v1Desc = generateVolume(v1Id);
    omHandle.proc->addVolume(v1Id, v1Desc);

    /* Add volume to DM */
    e = dmGroup[0]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
    ASSERT_TRUE(e == ERR_OK);
    ASSERT_TRUE(dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Offline);
    /* Any IO should fail */
    sendUpdateOnceMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOLUMEGROUP_NOT_OPEN);

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
    setupVolumeGroupHandleOnAm1(1);

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
    setupVolumeGroupHandleOnAm1(1);

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
    setupVolumeGroupHandleOnAm1(1);

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
    POLL_MS(v1->getFunctionalReplicasCnt() == dmGroup.size(), 1000, 800000);
    ASSERT_TRUE(v1->getFunctionalReplicasCnt() == dmGroup.size());


    /* Do more IO.  IO should succeed */
    doIo(10);

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(VolumeGroupFixture, singleWriterSingleReader) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    /* Create threed dms */
    createCluster(3);

    startAm2();

    /* Create read only volume group handel on second am */
    auto readonlyV1 = setupVolumeGroupHandleOnAm2(2);
    openVolumeReadonly(*readonlyV1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_DM_VOL_NOT_ACTIVATED);

    /* Open on group handle that is a coordinator */
    setupVolumeGroupHandleOnAm1(2);

    /* Do IO from coordinator and it should succeed */
    doIo(10);

    /* Open volume readonly and it should succeed */
    openVolumeReadonly(*readonlyV1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Reads on read only group handle should succeed */
    sendQueryCatalogMsg(*readonlyV1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Write on read only group handel should fail */
    sendUpdateOnceMsg(*readonlyV1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);

    /* Stop 1st dm, we should still be able to do reads */
    dmGroup[0]->stop();
    sendQueryCatalogMsg(*readonlyV1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Stop 2nd dm, we should still be able to do reads */
    dmGroup[1]->stop();
    sendQueryCatalogMsg(*readonlyV1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Stop 3rd dm, reads should fail */
    dmGroup[2]->stop();
    sendQueryCatalogMsg(*readonlyV1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);
    
    /* Do read from coordinator.  It should fail as well.  We do a read so that coordinator
     * knows which all dms are down.  This is needed because coordinator will do pings
     * which allows DMs to go through resyncs when they are up
     */
    sendQueryCatalogMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);

    /* Start 1st dm and wait for it be active */
    dmGroup[0]->start();
    doVolumeStateCheck(0, v1Id, fpi::Active);

    /* Reads should succeed now */
    sendQueryCatalogMsg(*readonlyV1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Close readonly volumegroup handle */
    waiter.reset(1);
    readonlyV1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(VolumeGroupFixture, multiWriter) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    /* Create threed dms */
    createCluster(3);
    setupVolumeGroupHandleOnAm1(2);

    /* Do more IO.  IO should succeed */
    doIo(10);


    /* Create volume group handel on second am */
    startAm2();
    auto am2VolumeHandle = setupVolumeGroupHandleOnAm2(2);
    openVolume(*am2VolumeHandle, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Any IO access via 2nd AM should succeed */
    sendQueryCatalogMsg(*am2VolumeHandle, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    sendUpdateOnceMsg(*am2VolumeHandle, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Any writes access via 1st AM should fail */
    sendUpdateOnceMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOLUMEGROUP_INVALID);

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Close readonly volumegroup handle */
    waiter.reset(1);
    am2VolumeHandle->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(VolumeGroupFixture, allDownFollowedBySequentialUp) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    /* Create two dms */
    createCluster(3);
    setupVolumeGroupHandleOnAm1(2);

    /* Do more IO.  IO should succeed */
    doIo(10);

    /* stop two dms */
    dmGroup[0]->stop();
    dmGroup[1]->stop();

#if 0
    /* Read should succeed */
    sendQueryCatalogMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
#endif

    /* Stop third dm */
    dmGroup[2]->stop();
    LOGNORMAL << "Stopped all the dms";

    /* Any Read should fail */
    sendQueryCatalogMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);

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

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(VolumeGroupFixture, sequenceIdMismatch) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    /* Create two dms */
    createCluster(3);
    setupVolumeGroupHandleOnAm1(2);

    /* Do more IO.  IO should succeed */
    doIo(10);

    /* stop one dms */
    dmGroup[0]->stop();
    
    /* IO should succeed */
    sendUpdateOnceMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* stop 2nd dms */
    dmGroup[1]->stop();

    /* IO will fail */
    sendUpdateOnceMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);

    /* Now all replicas should have different sequence ids */
    /* close and cleanup volume group handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Start the dms that were stopped */
    dmGroup[0]->start();
    dmGroup[1]->start();

    /* Recreate and open the volume group.  Though sequnce ids mismatch, open should
     * go through.
     */
    v1 = MAKE_SHARED<VolumeGroupHandle>(amHandle.proc, v1Id, 2);
    amHandle.proc->setVolumeHandle(v1.get());
    openVolume(*v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Any Read should succeed */
    sendQueryCatalogMsg(*v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Eventually writes should succeed as well */
    int nTries = 0;
    do {
        std::this_thread::sleep_for(std::chrono::seconds(nTries));
        sendUpdateOnceMsg(*v1, blobName, waiter);
        nTries++;
    } while (waiter.awaitResult() != ERR_OK && nTries < 3);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    doVolumeStateCheck(0, v1Id, fpi::Active);
    doVolumeStateCheck(1, v1Id, fpi::Active);
    doVolumeStateCheck(2, v1Id, fpi::Active);

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(DmGroupFixture, VolumeTargetCopy) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);
    /* Create two dms */
    createCluster(2);
    setupVolumeGroupHandleOnAm1(1);

    Waiter waiter(0);
    fds_volid_t v1Id(10);
    std::string blobName1 = "blob1";
    std::string blobName2 = "blob2";

    /* Copy volume with archive destination dm volume policy enabled */
    auto msg = MAKE_SHARED<fpi::CopyVolumeMsg>();
    msg->volId = 10;
    msg->destDmUuid = dmGroup[1]->proc->getSvcMgr()->getSelfSvcUuid().svc_uuid;
    msg->archivePolicy = true;
    auto reqMgr = omHandle.proc->getSvcMgr()->getSvcRequestMgr();
    auto req = reqMgr->newEPSvcRequest(dmGroup[0]->proc->getSvcMgr()->getSelfSvcUuid());
    req->onResponseCb([&waiter](EPSvcRequest* ee, const Error &e, StringPtr) {waiter.doneWith(e);});
    req->setPayload(fpi::CopyVolumeMsgTypeId,msg);
    req->setTimeoutMs(1000 * 10);
    waiter.reset(1);
    req->invoke();
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Copy volume without archiving destination dm's volume */
    msg->volId = 10;
    msg->destDmUuid = dmGroup[1]->proc->getSvcMgr()->getSelfSvcUuid().svc_uuid;
    msg->archivePolicy = false;
    reqMgr = omHandle.proc->getSvcMgr()->getSvcRequestMgr();
    req = reqMgr->newEPSvcRequest(dmGroup[0]->proc->getSvcMgr()->getSelfSvcUuid());
    req->onResponseCb([&waiter](EPSvcRequest* ee, const Error &e, StringPtr) {waiter.doneWith(e);});
    req->setPayload(fpi::CopyVolumeMsgTypeId,msg);
    req->setTimeoutMs(1000 * 10);
    waiter.reset(1);
    req->invoke();
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Close volumegroup handle */
    waiter.reset(1);
    v1->close([this, &waiter]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
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

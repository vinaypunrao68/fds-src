
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

struct TestAm : SvcProcess {
    TestAm(int argc, char *argv[], bool initAsModule)
    {
        handler_ = MAKE_SHARED<PlatNetSvcHandler>(this);
        auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler_);
        init(argc, argv, initAsModule, "platform.conf",
             "fds.am.", "am.log", nullptr, handler_, processor);
        REGISTER_FDSP_MSG_HANDLER_GENERIC(handler_, \
                                          fpi::AddToVolumeGroupCtrlMsg, addToVolumeGroup);
    }
    virtual int run() override
    {
        readyWaiter.done();
        shutdownGate_.waitUntilOpened();
        return 0;
    }
    void setVolumeHandle(VolumeGroupHandle *h) {
        vc_ = h;
    }
    void addToVolumeGroup(fpi::AsyncHdrPtr& asyncHdr,
                          fpi::AddToVolumeGroupCtrlMsgPtr& addMsg)
    {
        fds_volid_t volId(addMsg->groupId);
        vc_->handleAddToVolumeGroupMsg(
            addMsg,
            [this, asyncHdr](const Error& e,
                             const fpi::AddToVolumeGroupRespCtrlMsgPtr &payload) {
            asyncHdr->msg_code = e.GetErrno();
            handler_->sendAsyncResp(*asyncHdr,
                                    FDSP_MSG_TYPEID(fpi::AddToVolumeGroupRespCtrlMsg),
                                    *payload);
            });
    }
    PlatNetSvcHandlerPtr handler_;
    VolumeGroupHandle *vc_{nullptr};
};


struct DmGroupFixture : BaseTestFixture {
    using DmHandle = ProcessHandle<TestDm>;
    using OmHandle = ProcessHandle<TestOm>;
    using AmHandle = ProcessHandle<TestAm>;

    void createCluster(int numDms) {
        std::string fdsSrcPath;
        auto findRet = findFdsSrcPath(fdsSrcPath);
        fds_verify(findRet);

        std::string homedir = boost::filesystem::path(getenv("HOME")).string();
        std::string baseDir =  homedir + "/temp";
        setupDmClusterEnv(fdsSrcPath, baseDir);

        std::vector<std::string> roots;
        for (int i = 1; i <= numDms; i++) {
            roots.push_back(util::strformat("--fds-root=%s/node%d", baseDir.c_str(), i));
        }

        omHandle.start({"am",
                       roots[0],
                       "--fds.pm.platform_uuid=1024",
                       "--fds.pm.platform_port=7000",
                       "--fds.om.threadpool.num_threads=2"
                       });
        g_fdsprocess = omHandle.proc;
        g_cntrs_mgr = omHandle.proc->get_cntrs_mgr();

        amHandle.start({"am",
                       roots[0],
                       "--fds.pm.platform_uuid=2048",
                       "--fds.pm.platform_port=9850",
                       "--fds.am.threadpool.num_threads=3"
                       });

        int uuid = 2048;
        int port = 9850;
        dmGroup.resize(numDms);
        for (int i = 0; i < numDms; i++) {
            dmGroup[i].reset(new DmHandle);
            std::string platformUuid = util::strformat("--fds.pm.platform_uuid=%d", uuid);
            std::string platformPort = util::strformat("--fds.pm.platform_port=%d", port);
            dmGroup[i]->start({"dm",
                              roots[i],
                              platformUuid,
                              platformPort,
                              "--fds.dm.threadpool.num_threads=3",
                              "--fds.dm.qos.default_qos_threads=3",
                              "--fds.feature_toggle.common.enable_volumegrouping=true",
                              "--fds.feature_toggle.common.enable_timeline=false"
                              });

            uuid +=256;
            port += 10;
        }
    }

    void setupVolumeGroup(uint32_t quorumCnt)
    {
        /* Create a coordinator with quorum of 1*/
        v1 = MAKE_SHARED<VolumeGroupHandle>(amHandle.proc, v1Id, quorumCnt);
        amHandle.proc->setVolumeHandle(v1.get());

        /* Add DMT to AM with the DM group */
        std::vector<fpi::SvcUuid> dms;
        for (const auto &dm : dmGroup) {
            dms.push_back(dm->proc->getSvcMgr()->getSelfSvcUuid());
        }
        
        /* Add the DMT to om and am */
        std::string dmtData;
        dmt = DMT::newDMT(dms);
        dmt->getSerialized(dmtData);
        omHandle.proc->addDmt(dmt);
        Error e = amHandle.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                          nullptr,
                                                                          DMT_COMMITTED);
        ASSERT_TRUE(e == ERR_OK);

        /* Add volume to DM group */
        v1Desc = generateVolume(v1Id);
        omHandle.proc->addVolume(v1Id, v1Desc);
        for (uint32_t i = 0; i < dmGroup.size(); i++) {
            e = dmGroup[i]->proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                                 nullptr,
                                                                                 DMT_COMMITTED);
            ASSERT_TRUE(e == ERR_OK);
            e = dmGroup[i]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
            ASSERT_TRUE(e == ERR_OK);
            ASSERT_TRUE(dmGroup[i]->proc->getDataMgr()->\
                        getVolumeMeta(v1Id)->getState() == fpi::Offline);
        }

        /* open should succeed */
        openVolume(*v1, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    }

    SHPTR<VolumeDesc> generateVolume(const fds_volid_t &volId) {
        std::string name = "test" + std::to_string(volId.get());

        SHPTR<VolumeDesc> vdesc(new VolumeDesc(name, volId));

        vdesc->tennantId = volId.get();
        vdesc->localDomainId = volId.get();
        vdesc->globDomainId = volId.get();

        vdesc->volType = fpi::FDSP_VOL_S3_TYPE;
        // vdesc->volType = fpi::FDSP_VOL_BLKDEV_TYPE;
        vdesc->capacity = 10 * 1024;  // 10GB
        vdesc->maxQuota = 90;
        vdesc->redundancyCnt = 1;
        vdesc->maxObjSizeInBytes = MAX_OBJECT_SIZE;

        vdesc->writeQuorum = 1;
        vdesc->readQuorum = 1;
        vdesc->consisProtocol = fpi::FDSP_CONS_PROTO_EVENTUAL;

        vdesc->volPolicyId = 50;
        vdesc->archivePolicyId = 1;
        vdesc->mediaPolicy = fpi::FDSP_MEDIA_POLICY_HYBRID;
        vdesc->placementPolicy = 1;
        vdesc->appWorkload = fpi::FDSP_APP_S3_OBJS;
        vdesc->backupVolume = invalid_vol_id.get();

        vdesc->iops_assured = 1000;
        vdesc->iops_throttle = 5000;
        vdesc->relativePrio = 1;

        vdesc->fSnapshot = false;
        vdesc->srcVolumeId = invalid_vol_id;
        vdesc->lookupVolumeId = invalid_vol_id;
        vdesc->qosQueueId = invalid_vol_id;
        return vdesc;
    }

    void openVolume(VolumeGroupHandle &v, Waiter &w)
    {
        w.reset(1);
        v.open(MAKE_SHARED<fpi::OpenVolumeMsg>(),
               [this, &w](const Error &e, const fpi::OpenVolumeRspMsgPtr& resp) {
                   if (e == ERR_OK) {
                       sequenceId = resp->sequence_id;
                   }
                   w.doneWith(e);
               });
    }

    void doIo(uint32_t count)
    {
        /* Do more IO.  IO should succeed */
        for (uint32_t i = 0; i < count; i++) {
            sendUpdateOnceMsg(*v1, blobName, waiter);
            ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
            sendQueryCatalogMsg(*v1, blobName, waiter);
            ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        }
    }

    void sendUpdateOnceMsg(VolumeGroupHandle &v,
                           const std::string &blobName,
                           Waiter &w)
    {
        auto updateMsg = SvcMsgFactory::newUpdateCatalogOnceMsg(v.getGroupId(), blobName); 
        updateMsg->txId = txId++;
        updateMsg->sequence_id = ++sequenceId;
        w.reset(1);
        v.sendCommitMsg<fpi::UpdateCatalogOnceMsg>(
            FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceMsg),
            updateMsg,
            [&w](const Error &e, StringPtr) {
                w.doneWith(e);
            });
    }
    void sendStartBlobTxMsg(VolumeGroupHandle &v,
                            const std::string &blobName,
                            Waiter &w)
    {
        auto startMsg = SvcMsgFactory::newStartBlobTxMsg(v.getGroupId(), blobName); 
        startMsg->txId = txId++;
        w.reset(1);
        v.sendModifyMsg<fpi::StartBlobTxMsg>(
            FDSP_MSG_TYPEID(fpi::StartBlobTxMsg),
            startMsg,
            [&w](const Error &e, StringPtr) {
                w.doneWith(e);
            });
    }
    void sendQueryCatalogMsg(VolumeGroupHandle &v,
                             const std::string &blobName,
                             Waiter &w)
    {
        auto queryMsg= SvcMsgFactory::newQueryCatalogMsg(v.getGroupId(), blobName, 0);
        w.reset(1);
        v.sendReadMsg<fpi::QueryCatalogMsg>(
            FDSP_MSG_TYPEID(fpi::QueryCatalogMsg),
            queryMsg,
            [&w](const Error &e, StringPtr) {
                w.doneWith(e);
            });
    }
    void doGroupStateCheck(fds_volid_t vId)
    {
        for (uint32_t i = 0; i < dmGroup.size(); i++) {
            ASSERT_TRUE(dmGroup[i]->proc->getDataMgr()->\
                        getVolumeMeta(vId)->getState() == fpi::Active);
        }
    }

    void doVolumeStateCheck(int idx,
                            fds_volid_t volId,
                            fpi::ResourceState desiredState,
                            int32_t step = 500,
                            int32_t total = 8000)
    {
        POLL_MS(dmGroup[idx]->proc->getDataMgr()->getVolumeMeta(volId)->getState() == desiredState, step, total);
        ASSERT_TRUE(dmGroup[idx]->proc->getDataMgr()->getVolumeMeta(volId)->getState() == desiredState);
    }

    void doMigrationCheck(int idx, fds_volid_t volId)
    {
        ASSERT_TRUE(dmGroup[idx]->proc->getDataMgr()->counters->totalVolumesReceivedMigration.value() > 0);
        ASSERT_TRUE(dmGroup[idx]->proc->getDataMgr()->counters->totalMigrationsAborted.value() == 0);
    }
}

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
    std::string dmtData;
    dmt = DMT::newDMT({
                      dmGroup[0]->proc->getSvcMgr()->getSelfSvcUuid(),
                      });
    dmt->getSerialized(dmtData);
    Error e = amHandle.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                            nullptr,
                                                                            DMT_COMMITTED);
    ASSERT_TRUE(e == ERR_OK);
    e = dmGroup[0]->proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                         nullptr,
                                                                         DMT_COMMITTED);
    ASSERT_TRUE(e == ERR_OK);

    /* Open without volume being add to DM.  Open should fail */
    openVolume(*v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOL_NOT_FOUND);

    /* Generate volume descriptor */
    v1Desc = generateVolume(v1Id);
    omHandle.proc->addVolume(v1Id, v1Desc);

    /* Add volume to DM */
    e = dmGroup[0]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
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

TEST_F(DmGroupFixture, VolumeTargetCopy) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);
    /* Create two dms */
    createCluster(2);
    setupVolumeGroup(1);

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

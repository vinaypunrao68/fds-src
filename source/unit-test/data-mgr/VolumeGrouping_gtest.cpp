/* Copyright 2015 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <util/stringutils.h>
#include <concurrency/RwLock.h>
#include <testlib/TestFixtures.h>
#include <testlib/TestOm.hpp>
#include <testlib/TestDm.hpp>
#include <testlib/ProcessHandle.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <net/VolumeGroupHandle.h>

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

    void create(int numDms) {
        int ret;
        ret = std::system("rm -rf /fds/sys-repo/dm-names/*");
        ret = std::system("rm -rf /fds/node1/sys-repo/dm-names/*");
        ret = std::system("rm -rf /fds/node2/sys-repo/dm-names/*");

        omHandle.start({"am",
            "--fds-root=/fds/node1",
            "--fds.pm.platform_uuid=1024",
            "--fds.pm.platform_port=7000",
            "--fds.om.threadpool.num_threads=2"
        });
        g_fdsprocess = omHandle.proc;
        g_cntrs_mgr = omHandle.proc->get_cntrs_mgr();
        auto h= omHandle.proc->getSvcMgr()->getSvcRequestHandler();
        h->updateHandler(
            FDSP_MSG_TYPEID(fpi::SetVolumeGroupCoordinatorMsg),
            [h](fpi::AsyncHdrPtr &hdr, StringPtr &payload) {
                h->sendAsyncResp(*hdr,
                                 FDSP_MSG_TYPEID(fpi::EmptyMsg),
                                 fpi::EmptyMsg());
            });

        amHandle.start({"am",
            "--fds-root=/fds/node1",
            "--fds.pm.platform_uuid=2048",
            "--fds.pm.platform_port=9850",
            "--fds.am.threadpool.num_threads=3"
        });

        int uuid = 2048;
        int port = 9850;
        dmGroup.resize(numDms);
        for (int i = 0; i < numDms; i++) {
            dmGroup[i].reset(new DmHandle);
            std::string root = util::strformat("--fds-root=/fds/node%d", i+1);
            std::string platformUuid = util::strformat("--fds.pm.platform_uuid=%d", uuid);
            std::string platformPort = util::strformat("--fds.pm.platform_port=%d", port);
            dmGroup[i]->start({"dm",
                              root,
                              platformUuid,
                              platformPort,
                              "--fds.dm.threadpool.num_threads=3",
                              "--fds.dm.qos.default_qos_threads=3",
                              "--fds.feature_toggle.common.enable_volumegrouping=true"
                              });

            uuid +=256;
            port += 10;
        }
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
    void openVolume(VolumeGroupHandle &v, Waiter &waiter)
    {
        waiter.reset(1);
        v.open(MAKE_SHARED<fpi::OpenVolumeMsg>(),
               [&waiter](const Error &e, const fpi::OpenVolumeRspMsgPtr&) {
                   waiter.doneWith(e);
               });
    }
    void sendUpdateOnceMsg(VolumeGroupHandle &v,
                           const std::string &blobName,
                           Waiter &waiter)
    {
        auto updateMsg = SvcMsgFactory::newUpdateCatalogOnceMsg(v.getGroupId(), blobName); 
        updateMsg->txId = txId++;
        updateMsg->sequence_id = sequenceId++;
        waiter.reset(1);
        v.sendCommitMsg<fpi::UpdateCatalogOnceMsg>(
            FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceMsg),
            updateMsg,
            [&waiter](const Error &e, StringPtr) {
                waiter.doneWith(e);
            });
    }
    void sendStartBlobTxMsg(VolumeGroupHandle &v,
                       const std::string &blobName,
                       Waiter &waiter)
    {
        auto startMsg = SvcMsgFactory::newStartBlobTxMsg(v.getGroupId(), blobName); 
        startMsg->txId = txId++;
        waiter.reset(1);
        v.sendModifyMsg<fpi::StartBlobTxMsg>(
            FDSP_MSG_TYPEID(fpi::StartBlobTxMsg),
            startMsg,
            [&waiter](const Error &e, StringPtr) {
                waiter.doneWith(e);
            });
    }
    void sendQueryCatalogMsg(VolumeGroupHandle &v,
                             const std::string &blobName,
                             Waiter &waiter)
    {
        auto queryMsg= SvcMsgFactory::newQueryCatalogMsg(v.getGroupId(), blobName, 0);
        waiter.reset(1);
        v.sendReadMsg<fpi::QueryCatalogMsg>(
            FDSP_MSG_TYPEID(fpi::QueryCatalogMsg),
            queryMsg,
            [&waiter](const Error &e, StringPtr) {
                waiter.doneWith(e);
            });
    }
    void doGroupStateCheck(fds_volid_t vId)
    {
        for (uint32_t i = 0; i < dmGroup.size(); i++) {
            ASSERT_TRUE(dmGroup[i]->proc->getDataMgr()->\
                        getVolumeMeta(vId)->getState() == fpi::Active);
        }
    }

    OmHandle                omHandle;
    AmHandle                amHandle;
    std::vector<std::unique_ptr<DmHandle>> dmGroup;
    int64_t                 sequenceId {0};
    int64_t                 txId {0};
};

TEST_F(DmGroupFixture, singledm) {
    /* Start with one dm */
    create(1);

    Waiter waiter(0);
    fds_volid_t v1Id(10);
    std::string blobName = "blob1";

    /* Create a DM group */
    /* Create a coordinator */
    VolumeGroupHandle v1(amHandle.proc, v1Id, 1);
    amHandle.proc->setVolumeHandle(&v1);

    /* Open the group without DMT.  Open should fail */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);

    /* Add DMT */
    std::string dmtData;
    auto dmt = DMT::newDMT({
                           dmGroup[0]->proc->getSvcMgr()->getSelfSvcUuid(),
                           });
    dmt->getSerialized(dmtData);
    Error e = amHandle.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                            nullptr,
                                                                            DMT_COMMITTED);
    ASSERT_TRUE(e == ERR_OK);

    /* Open without volume being add to DM.  Open should fail */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOL_NOT_FOUND);

    /* Add volume to DM */
    auto v1Desc = generateVolume(v1Id);
    e = dmGroup[0]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
    ASSERT_TRUE(e == ERR_OK);
    ASSERT_TRUE(dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Offline);
    /* Any IO should fail */
    sendUpdateOnceMsg(v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOLUMEGROUP_DOWN);

    /* Now open should succeed */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Do some io. After Io is done, every volume replica must have same state */
    sendUpdateOnceMsg(v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    ASSERT_TRUE(dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Active);
    sendQueryCatalogMsg(v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Bring a dm down */
    dmGroup[0]->stop();
    /* Do more IO.  IO should fail */
    sendQueryCatalogMsg(v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);
    
    /* Close volumegroup handle */
    waiter.reset(1);
    v1.close([&waiter]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(DmGroupFixture, staticio_restarts) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);
    /* Create two dms */
    create(2);

    Waiter waiter(0);
    fds_volid_t v1Id(10);
    std::string blobName = "blob1";

    /* Create a coordinator with quorum of 1*/
    VolumeGroupHandle v1(amHandle.proc, v1Id, 1);
    amHandle.proc->setVolumeHandle(&v1);

    /* Add DMT to AM with the DM group */
    std::string dmtData;
    auto dmt = DMT::newDMT({
                           dmGroup[0]->proc->getSvcMgr()->getSelfSvcUuid(),
                           dmGroup[1]->proc->getSvcMgr()->getSelfSvcUuid(),
                           });
    dmt->getSerialized(dmtData);
    Error e = amHandle.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                            nullptr,
                                                                            DMT_COMMITTED);
    ASSERT_TRUE(e == ERR_OK);

    /* Open without volume being add to DM.  Open should fail */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOL_NOT_FOUND);

    /* Add volume to DM group */
    auto v1Desc = generateVolume(v1Id);
    for (uint32_t i = 0; i < dmGroup.size(); i++) {
        e = dmGroup[i]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
        ASSERT_TRUE(e == ERR_OK);
        ASSERT_TRUE(dmGroup[i]->proc->getDataMgr()->\
                    getVolumeMeta(v1Id)->getState() == fpi::Offline);
    }
    /* Any IO should fail prior open */
    sendUpdateOnceMsg(v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOLUMEGROUP_DOWN);

    /* Now open should succeed */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Do some io. After Io is done, every volume replica must have same state */
    for (uint32_t i = 0; i < 10; i++) {
        sendUpdateOnceMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        doGroupStateCheck(v1Id);
    }

    /* Bring a dm down */
    dmGroup[0]->stop();
    /* Do more IO.  IO should succeed */
    for (uint32_t i = 0; i < 10; i++) {
        blobName = "blob" + std::to_string(i);
        sendUpdateOnceMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    }
    /* Send few more non-commit updates so that we have active Txs */
    for (uint32_t i = 0; i < 5; i++) {
        sendStartBlobTxMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    }

    /* Bring 1st dm up again */
    dmGroup[0]->start();
    /* Adding the volume manually */
    v1Desc->setCoordinatorId(amHandle.proc->getSvcMgr()->getSelfSvcUuid());
    e = dmGroup[0]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
    ASSERT_TRUE(e == ERR_OK);
    
    /* Keep doing IO for maximum of 10 seconds */
    for (uint32_t i = 0;
         (i < 100 &&
          dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() != fpi::Active);
         i++) {
        sendUpdateOnceMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        if (i % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    /* By now sync must complete */
    ASSERT_TRUE(dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Active);

    /* Do more IO.  IO should succeed */
    for (uint32_t i = 0; i < 10; i++) {
        sendUpdateOnceMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    }

    /* Close volumegroup handle */
    waiter.reset(1);
    v1.close([&waiter]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(DmGroupFixture, activeio_restart) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);
    /* Create two dms */
    create(2);

    Waiter waiter(0);
    fds_volid_t v1Id(10);
    std::string blobName = "blob1";

    /* Create a coordinator with quorum of 1*/
    VolumeGroupHandle v1(amHandle.proc, v1Id, 1);
    amHandle.proc->setVolumeHandle(&v1);

    /* Add DMT to AM with the DM group */
    std::string dmtData;
    auto dmt = DMT::newDMT({
                           dmGroup[0]->proc->getSvcMgr()->getSelfSvcUuid(),
                           dmGroup[1]->proc->getSvcMgr()->getSelfSvcUuid(),
                           });
    dmt->getSerialized(dmtData);
    Error e = amHandle.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                            nullptr,
                                                                            DMT_COMMITTED);
    ASSERT_TRUE(e == ERR_OK);

    /* Add volume to DM group */
    auto v1Desc = generateVolume(v1Id);
    for (uint32_t i = 0; i < dmGroup.size(); i++) {
        e = dmGroup[i]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
        ASSERT_TRUE(e == ERR_OK);
        ASSERT_TRUE(dmGroup[i]->proc->getDataMgr()->\
                    getVolumeMeta(v1Id)->getState() == fpi::Offline);
    }
    /* open should succeed */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Start io thread */
    bool ioAbort = false;
    std::thread iothread([this, &ioAbort, &v1, &blobName, &waiter]() {
        int ioCnt = 0;
        while (!ioAbort) {
            sendUpdateOnceMsg(v1, blobName, waiter);
            ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
            sendQueryCatalogMsg(v1, blobName, waiter);
            ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
            ioCnt++;
            if (ioCnt % 10) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });

    /* Set the coordinator for the volume */
    v1Desc->setCoordinatorId(amHandle.proc->getSvcMgr()->getSelfSvcUuid());
    for (int i = 0; i < 4; i++) {
        /* Bring a dm down */
        dmGroup[0]->stop();

        /* Bring 1st dm up again */
        dmGroup[0]->start();
        /* Add the volume manually */
        e = dmGroup[0]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
        ASSERT_TRUE(e == ERR_OK);
        POLL_MS(dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Active, 500, 8000);
        ASSERT_TRUE(dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Active);
    }

    ioAbort = true;
    iothread.join();

    /* Close volumegroup handle */
    waiter.reset(1);
    v1.close([&waiter]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

TEST_F(DmGroupFixture, domain_reboot) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);
    /* Create two dms */
    create(2);

    Waiter waiter(0);
    fds_volid_t v1Id(10);
    std::string blobName = "blob1";

    /* Create a coordinator with quorum of 1*/
    VolumeGroupHandle v1(amHandle.proc, v1Id, 1);
    amHandle.proc->setVolumeHandle(&v1);

    /* Add DMT to AM with the DM group */
    std::string dmtData;
    auto dmt = DMT::newDMT({
                           dmGroup[0]->proc->getSvcMgr()->getSelfSvcUuid(),
                           dmGroup[1]->proc->getSvcMgr()->getSelfSvcUuid(),
                           });
    dmt->getSerialized(dmtData);
    Error e = amHandle.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                            nullptr,
                                                                            DMT_COMMITTED);
    ASSERT_TRUE(e == ERR_OK);

    /* Add volume to DM group */
    auto v1Desc = generateVolume(v1Id);
    for (uint32_t i = 0; i < dmGroup.size(); i++) {
        e = dmGroup[i]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
        ASSERT_TRUE(e == ERR_OK);
        ASSERT_TRUE(dmGroup[i]->proc->getDataMgr()->\
                    getVolumeMeta(v1Id)->getState() == fpi::Offline);
    }
    /* open should succeed */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Do more IO.  IO should succeed */
    for (uint32_t i = 0; i < 10; i++) {
        sendUpdateOnceMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    }

    /* Close volumegroup handle */
    waiter.reset(1);
    v1.close([&waiter]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Stop all the DMs */
    for (uint32_t i = 0; i < dmGroup.size(); i++) {
        dmGroup[i]->stop();
    }

    /* Start all dms and manually add volumes */
    for (uint32_t i = 0; i < dmGroup.size(); i++) {
        dmGroup[i]->start();
        e = dmGroup[i]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
        ASSERT_TRUE(e == ERR_OK);
    }

    /* Open the handle */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Do more IO.  IO should succeed */
    for (uint32_t i = 0; i < 10; i++) {
        sendUpdateOnceMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    }

    /* Close volumegroup handle */
    waiter.reset(1);
    v1.close([&waiter]() { waiter.doneWith(ERR_OK); });
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    g_fdslog = new fds_log("volumegtest");
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>()->default_value(1), "puts count");
    DmGroupFixture::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

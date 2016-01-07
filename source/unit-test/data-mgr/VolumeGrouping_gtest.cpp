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
#include <net/VolumeGroupHandle.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;
#define MAX_OBJECT_SIZE 1024 * 1024 * 2

struct TestAm : SvcProcess {
    TestAm(int argc, char *argv[], bool initAsModule)
    {
        auto handler = boost::make_shared<PlatNetSvcHandler>(this);
        auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);
        init(argc, argv, initAsModule, "platform.conf",
             "fds.am.", "am.log", nullptr, handler, processor);
    }
    virtual int run() override
    {
        readyWaiter.done();
        shutdownGate_.waitUntilOpened();
        return 0;
    }
};

struct Waiter : concurrency::TaskStatus {
    using concurrency::TaskStatus::TaskStatus;
    void doneWith(const Error &e) {
        if (error == ERR_OK) {
            error = e;
        }
        done();
    }
    void reset(uint32_t numTasks) {
        error = ERR_OK;
        concurrency::TaskStatus::reset(numTasks);
    }
    Error awaitResult() {
        await();
        return error;
    }
    Error awaitResult(ulong timeout) {
        await(timeout);
        return error;
    }
    Error error;
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
               [&waiter](const Error &e) {
                   waiter.doneWith(e);
               });
    }
    void sendUpdateOnceMsg(VolumeGroupHandle &v,
                           const std::string &blobName,
                           int64_t txId,
                           Waiter &waiter)
    {
        auto updateMsg = SvcMsgFactory::newUpdateCatalogOnceMsg(v.getGroupId(), blobName); 
        updateMsg->txId = txId;
        waiter.reset(1);
        v.sendWriteMsg<fpi::UpdateCatalogOnceMsg>(
            FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceMsg),
            updateMsg,
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
};

TEST_F(DmGroupFixture, DISABLED_singledm) {
    /* Start with one dm */
    create(1);

    Waiter waiter(0);
    fds_volid_t v1Id(10);
    std::string blobName = "blob1";

    /* Create a DM group */
    /* Create a coordinator */
    VolumeGroupHandle v1(amHandle.proc, v1Id, 1);

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
    sendUpdateOnceMsg(v1, blobName, 1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOLUMEGROUP_DOWN);

    /* Now open should succeed */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Do some io. After Io is done, every volume replica must have same state */
    sendUpdateOnceMsg(v1, blobName, 1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    ASSERT_TRUE(dmGroup[0]->proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Active);
    sendQueryCatalogMsg(v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Bring a dm down */
    dmGroup[0]->stop();
    /* Do more IO.  IO should fail */
    sendQueryCatalogMsg(v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);
}

TEST_F(DmGroupFixture, multidm) {
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);
    /* Create two dms */
    create(2);

    Waiter waiter(0);
    fds_volid_t v1Id(10);
    std::string blobName = "blob1";
    int64_t curTxId = 1;

    /* Create a coordinator with quorum of 1*/
    VolumeGroupHandle v1(amHandle.proc, v1Id, 1);

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
    sendUpdateOnceMsg(v1, blobName, curTxId, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_VOLUMEGROUP_DOWN);

    /* Now open should succeed */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Do some io. After Io is done, every volume replica must have same state */
    for (uint32_t i = 0; i < 10; i++, curTxId++) {
        sendUpdateOnceMsg(v1, blobName, curTxId, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        doGroupStateCheck(v1Id);
    }

    /* Bring a dm down */
    dmGroup[0]->stop();
    /* Do more IO.  IO should succeed */
    for (uint32_t i = 0; i < 10; i++, curTxId++) {
        sendUpdateOnceMsg(v1, blobName, curTxId, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        sendQueryCatalogMsg(v1, blobName, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    }

    /* Bring 2nd dm down */
    dmGroup[1]->stop();
    /* Do more IO.  IO should fail */
    sendQueryCatalogMsg(v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);
}

#if 0
TEST(ProcHandle, DISABLED_test1) {
    ProcessHandle<TestOm>          om("om", "/fds", 1024, 7000);
    g_fdsprocess = om.proc;
    g_cntrs_mgr = om.proc->get_cntrs_mgr();

    ProcessHandle<TestDm> dm1("dm" , "/fds/node1", 2048, 9850);
    dm1.stop();
}
#endif

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

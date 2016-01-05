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

    virtual void SetUp() override {
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

        dmHandle1.start({"dm",
            "--fds-root=/fds/node1",
            "--fds.pm.platform_uuid=2048",
            "--fds.pm.platform_port=9850",
            "--fds.dm.threadpool.num_threads=3",
            "--fds.dm.qos.default_qos_threads=3",
            "--fds.feature_toggle.common.enable_volumegrouping=true"
        });
        // dmHandles[1].start("dm" , "/fds/node2", 2064, 9860);
        // dmHandles[2].start("dm" , "/fds/node3", 2080, 9870);
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

    OmHandle                omHandle;
    AmHandle                amHandle;
    DmHandle                dmHandle1;
};

TEST_F(DmGroupFixture, basicio) {
    Waiter waiter(0);
    fds_volid_t v1Id(10);

    /* Create a DM group */
    /* Create a coordinator */
    VolumeGroupHandle v1(amHandle.proc, v1Id, 1);

    /* Open the group without DMT.  Open should fail */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() != ERR_OK);

    /* Add DMT */
    std::string dmtData;
    auto dmt = DMT::newDMT({
                           dmHandle1.proc->getSvcMgr()->getSelfSvcUuid(),
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
    e = dmHandle1.proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
    ASSERT_TRUE(e == ERR_OK);
    ASSERT_TRUE(dmHandle1.proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Offline);

    /* Now open should succeed */
    openVolume(v1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    ASSERT_TRUE(dmHandle1.proc->getDataMgr()->getVolumeMeta(v1Id)->getState() == fpi::Active);


    /* Do some io. After Io is done, every volume replica must have same state */
    std::string blobName = "blob1";
    sendUpdateOnceMsg(v1, blobName, 1, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    sendQueryCatalogMsg(v1, blobName, waiter);
    ASSERT_TRUE(waiter.awaitResult() == ERR_OK);

    /* Bring a dm down */
    /* Do more IO.  IO should succeed */
    
}

TEST(ProcHandle, DISABLED_test1) {
    ProcessHandle<TestOm>          om("om", "/fds", 1024, 7000);
    g_fdsprocess = om.proc;
    g_cntrs_mgr = om.proc->get_cntrs_mgr();

    ProcessHandle<TestDm> dm1("dm" , "/fds/node1", 2048, 9850);
    dm1.stop();
}


#if 0
TEST_F(ClusterFixture, DISABLED_test_quicksync)
{
    // Temporary hack
    // TODO(Rao): Get rid of this by putting perftraceing under PERF macro
    ProcessHandle<Om>          om("Om", "/fds", 1024, 7000);
    g_fdsprocess = om.proc.get();
    g_cntrs_mgr = om.proc->get_cntrs_mgr();

    ProcessHandle<AmProcess>   am("Om", "/fds", 1024, 7000);
    ProcessHandle<TestDm>   dm1("TestDm", "/fds", 1024, 7000);
    ProcessHandle<TestDm>   dm2("TestDm", "/fds/node1", 2048, 9850);
    ProcessHandle<TestDm>   dm3("TestDm", "/fds/node2", 4096, 10850);

    fds_volid_t v(10);
    fpi::VolumeGroupInfo volumeGroup;
    volumeGroup.groupId = v.get();
    volumeGroup.version = 0;
    volumeGroup.functionalReplicas.push_back(dm1.proc->getSvcMgr()->getSelfSvcUuid());
    volumeGroup.functionalReplicas.push_back(dm2.proc->getSvcMgr()->getSelfSvcUuid());
    volumeGroup.functionalReplicas.push_back(dm3.proc->getSvcMgr()->getSelfSvcUuid());

    ASSERT_EQ(dm1.proc->addVolume(v), ERR_OK);
    ASSERT_EQ(dm2.proc->addVolume(v), ERR_OK);
    ASSERT_EQ(dm3.proc->addVolume(v), ERR_OK);
    am.proc->attachVolume(volumeGroup);
    
    int nPuts = 20;
    for (int i = 0; i < nPuts; i++) {
        concurrency::TaskStatus s(3);
        am.proc->putBlob(v,
                        [&s](const Error& e, StringPtr resp) {
                        GLOGNOTIFY << "Received response: " << e;
                        s.done();
                        });
        s.await();
        if (i==10) {
            dm2.stop();
        }
    }
    sleep(7);

    std::cout << "Starting sync";
    dm2.start();
    ASSERT_EQ(dm2.proc->addVolume(v), ERR_OK);
    dm2.proc->getVolume(v)->forceQuickSync(am.proc->getSvcMgr()->getSelfSvcUuid());
    sleep(4);

    for (int i = 0; i < nPuts; i++) {
        concurrency::TaskStatus s(3);
        am.proc->putBlob(v,
                        [&s](const Error& e, StringPtr resp) {
                        GLOGNOTIFY << "Received response: " << e;
                        s.done();
                        });
        s.await();
    }
    sleep(5);
    GLOGNOTIFY << "Exiting from test";
}

TEST_F(ClusterFixture, testquicksync_activeio)
{
    // Temporary hack
    // TODO(Rao): Get rid of this by putting perftraceing under PERF macro
    ProcessHandle<Om>          om("Om", "/fds", 1024, 7000);
    g_fdsprocess = om.proc.get();
    g_cntrs_mgr = om.proc->get_cntrs_mgr();

    ProcessHandle<AmProcess>   am("Om", "/fds", 1024, 7000);
    ProcessHandle<TestDm>   dm1("TestDm", "/fds", 1024, 7000);
    ProcessHandle<TestDm>   dm2("TestDm", "/fds/node1", 2048, 9850);
    ProcessHandle<TestDm>   dm3("TestDm", "/fds/node2", 4096, 10850);

    // g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    fds_volid_t v(10);
    fpi::VolumeGroupInfo volumeGroup;
    volumeGroup.groupId = v.get();
    volumeGroup.version = 0;
    volumeGroup.functionalReplicas.push_back(dm1.proc->getSvcMgr()->getSelfSvcUuid());
    volumeGroup.functionalReplicas.push_back(dm2.proc->getSvcMgr()->getSelfSvcUuid());
    volumeGroup.functionalReplicas.push_back(dm3.proc->getSvcMgr()->getSelfSvcUuid());

    ASSERT_EQ(dm1.proc->addVolume(v), ERR_OK);
    ASSERT_EQ(dm2.proc->addVolume(v), ERR_OK);
    ASSERT_EQ(dm3.proc->addVolume(v), ERR_OK);
    am.proc->attachVolume(volumeGroup);
    
    int nPuts = 10000;
    int stopCnt = 10;
    int startCnt = 1000;
    for (int i = 0; i < nPuts; i++) {
        concurrency::TaskStatus s(3);
        am.proc->putBlob(v,
                        [&s](const Error& e, StringPtr resp) {
                        s.done();
                        });
        s.await();
        if (i==stopCnt) {
            dm2.stop();
            GLOGNOTIFY << "Completed " << i << " reqs. Stopped dm2";
        }
        if (i==startCnt) {
            GLOGNOTIFY << "Completed " << i << " reqs. Started dm2";
            dm2.start();
            ASSERT_EQ(dm2.proc->addVolume(v), ERR_OK);
            dm2.proc->getVolume(v)->forceQuickSync(am.proc->getSvcMgr()->getSelfSvcUuid());
        }
        if (i > startCnt && i % 10 == 0) {
           std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    GLOGNOTIFY << "Received all responses";

    sleep(5);
    dm1.stop();
    dm3.stop();
    am.stop();
    om.stop();
    dm2.stop();



    GLOGNOTIFY << "Exiting from test";
}
TEST_F(ClusterFixture, DISABLED_test1)
{
    // Temporary hack
    // TODO(Rao): Get rid of this by putting perftraceing under PERF macro
    ProcessHandle<Om>          om("Om", "/fds", 1024, 7000);
    g_fdsprocess = om.proc.get();
    g_cntrs_mgr = om.proc->get_cntrs_mgr();

    ProcessHandle<AmProcess>   am("Om", "/fds", 1024, 7000);
    ProcessHandle<TestDm>   dm1("TestDm", "/fds", 1024, 7000);
    ProcessHandle<TestDm>   dm2("TestDm", "/fds/node1", 2048, 9850);
    ProcessHandle<TestDm>   dm3("TestDm", "/fds/node2", 4096, 10850);

    fds_volid_t v(10);
    fpi::VolumeGroupInfo volumeGroup;
    volumeGroup.groupId = v.get();
    volumeGroup.version = 0;
    volumeGroup.functionalReplicas.push_back(dm1.proc->getSvcMgr()->getSelfSvcUuid());
    volumeGroup.functionalReplicas.push_back(dm2.proc->getSvcMgr()->getSelfSvcUuid());
    volumeGroup.functionalReplicas.push_back(dm3.proc->getSvcMgr()->getSelfSvcUuid());

    ASSERT_EQ(dm1.proc->addVolume(v), ERR_OK);
    ASSERT_EQ(dm2.proc->addVolume(v), ERR_OK);
    ASSERT_EQ(dm3.proc->addVolume(v), ERR_OK);
    am.proc->attachVolume(volumeGroup);
    
    int nPuts =  this->getArg<int>("puts-cnt");
    for (int i = 0; i < nPuts; i++) {
        concurrency::TaskStatus s(3);
        am.proc->putBlob(v,
                        [&s](const Error& e, StringPtr resp) {
                        GLOGNOTIFY << "Received response: " << e;
                        s.done();
                        });
        s.await();
        if (i==10) {
            dm2.stop();
        }
    }
    sleep(5);
    GLOGNOTIFY << "Exiting from test";
}

TEST_F(ClusterFixture, DISABLED_basic)
{
    omProc.reset(new Om(argc_, argv_, true));
    std::thread t1([&] { omProc->main(); });
    omProc->getReadyWaiter().await();

    // Temporary hack
    // TODO(Rao): Get rid of this by putting perftraceing under PERF macro
    g_fdsprocess = omProc.get();
    g_cntrs_mgr = omProc->get_cntrs_mgr();

    dmProc.reset(new TestDm(argc_, argv_, true));
    std::thread t2([&] { dmProc->main(); });
    dmProc->getReadyWaiter().await();

    amProc.reset(new AmProcess(argc_, argv_, true));
    std::thread t3([&] { amProc->main(); });
    amProc->getReadyWaiter().await();

    char* args2[] = {"DmProc", "--fds-root=/fds/node1",
                    "--fds.pm.platform_uuid=2048",
                    "--fds.pm.platform_port=9850"};
    dmProc2.reset(new TestDm(4, args2, true));
    std::thread t4([&] { dmProc2->main(); });
    dmProc2->getReadyWaiter().await();

    char* args3[] = {"DmProc", "--fds-root=/fds/node2",
                    "--fds.pm.platform_uuid=4096",
                    "--fds.pm.platform_port=10850"};
    dmProc3.reset(new TestDm(4, args3, true));
    std::thread t5([&] { dmProc3->main(); });
    dmProc3->getReadyWaiter().await();

    fds_volid_t v(10);
    fpi::VolumeGroupInfo volumeGroup;
    volumeGroup.groupId = v.get();
    volumeGroup.version = 0;
    volumeGroup.functionalReplicas.push_back(dmProc->getSvcMgr()->getSelfSvcUuid());
    volumeGroup.functionalReplicas.push_back(dmProc2->getSvcMgr()->getSelfSvcUuid());
    volumeGroup.functionalReplicas.push_back(dmProc3->getSvcMgr()->getSelfSvcUuid());

    ASSERT_EQ(dmProc->addVolume(v), ERR_OK);
    ASSERT_EQ(dmProc2->addVolume(v), ERR_OK);
    ASSERT_EQ(dmProc3->addVolume(v), ERR_OK);
    amProc->attachVolume(volumeGroup);
    
    int nPuts =  this->getArg<int>("puts-cnt");
    for (int i = 0; i < nPuts; i++) {
        concurrency::TaskStatus s(3);
        amProc->putBlob(v,
                        [&s](const Error& e, StringPtr resp) {
                        GLOGNOTIFY << "Received response: " << e;
                        s.done();
                        });
        s.await();
    }
    GLOGNOTIFY << "Exiting from test";

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

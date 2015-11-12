/* Copyright 2015 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testlib/TestFixtures.h>
#include <Dm.h>
#include <Om.h>
#include <Am.h>

using ::testing::AtLeast;
using ::testing::Return;

template<class T>
struct ProcHandle {
};

struct ClusterFixture : BaseTestFixture {
    virtual void SetUp() override {
    std::system("rm -rf /fds/sys-repo/dm-names/*");
#if 0
        omProc.reset(new fds::Om(argc_, argv_, true));
        std::thread t1([&] { omProc->main(); });
        omProc->getReadyWaiter().await();

        dmProc.reset(new fds::DmProcess(argc_, argv_, true));
        std::thread t2([&] { dmProc->main(); });
        dmProc->getReadyWaiter().await();

        amProc.reset(new fds::AmProcess(argc_, argv_, true));
        std::thread t3([&] { amProc->main(); });
        amProc->getReadyWaiter().await();

        GLOGNOTIFY << "All procs are ready";
#endif
    }
    std::unique_ptr<fds::DmProcess> dmProc;
    std::unique_ptr<fds::DmProcess> dmProc2;
    std::unique_ptr<fds::AmProcess> amProc;
    std::unique_ptr<fds::Om> omProc;
};

// TODO(Rao): Svc map checker

TEST(Leveldb, DISABLED_batchcheck)
{
    leveldb::DB* tempDb;
    leveldb::Options options;
    options.create_if_missing = true;
    auto status = leveldb::DB::Open(options, "/fds/sys-repo/dm-names/10", &tempDb);
    ASSERT_TRUE(status.ok());

    leveldb::WriteBatch batch;
    batch.Put("name", "rao");
    batch.Put("age", "32");
    status = tempDb->Write(leveldb::WriteOptions(), &batch);
    ASSERT_TRUE(status.ok());
}

TEST_F(ClusterFixture, basic)
{
    omProc.reset(new fds::Om(argc_, argv_, true));
    std::thread t1([&] { omProc->main(); });
    omProc->getReadyWaiter().await();

    // Temporary hack
    // TODO(Rao): Get rid of this by putting perftraceing under PERF macro
    g_fdsprocess = omProc.get();
    g_cntrs_mgr = omProc->get_cntrs_mgr();

    dmProc.reset(new fds::DmProcess(argc_, argv_, true));
    std::thread t2([&] { dmProc->main(); });
    dmProc->getReadyWaiter().await();

    amProc.reset(new fds::AmProcess(argc_, argv_, true));
    std::thread t3([&] { amProc->main(); });
    amProc->getReadyWaiter().await();

    char* args[] = {"DmProc", "--fds-root=/fds/node1",
                    "--fds.pm.platform_uuid=4096",
                    "--fds.pm.platform_port=10850"};
    dmProc2.reset(new fds::DmProcess(4, args, true));
    std::thread t4([&] { dmProc2->main(); });
    dmProc2->getReadyWaiter().await();

    fds_volid_t v(10);
    fpi::VolumeGroupInfo volumeGroup;
    volumeGroup.groupId = v.get();
    volumeGroup.version = 0;
    volumeGroup.functionalReplicas.push_back(dmProc->getSvcMgr()->getSelfSvcUuid());
    volumeGroup.functionalReplicas.push_back(dmProc2->getSvcMgr()->getSelfSvcUuid());

    ASSERT_EQ(dmProc->addVolume(v), ERR_OK);
    ASSERT_EQ(dmProc2->addVolume(v), ERR_OK);
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

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    g_fdslog = new fds_log("volumegtest");
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>()->default_value(1), "puts count");
    ClusterFixture::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

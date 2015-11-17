/* Copyright 2015 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testlib/TestFixtures.h>
#include <util/stringutils.h>
#include <Dm.h>
#include <Om.h>
#include <Am.h>

using ::testing::AtLeast;
using ::testing::Return;

struct ClusterFixture : BaseTestFixture {
    virtual void SetUp() override {
    std::system("rm -rf /fds/sys-repo/dm-names/*");
    std::system("rm -rf /fds/node1/sys-repo/dm-names/*");
    std::system("rm -rf /fds/node2/sys-repo/dm-names/*");
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
    std::unique_ptr<fds::DmProcess> dmProc3;
    std::unique_ptr<fds::AmProcess> amProc;
    std::unique_ptr<fds::Om> omProc;
};
template <class T>
struct ProcessHandle
{
    std::unique_ptr<std::thread>        thread;
    std::unique_ptr<T>                  proc;
    std::vector<std::string>            args;

    ProcessHandle(const std::string &processName,
                  const std::string &fds_root,
                  int64_t platformUuid,
                  int32_t platformPort)
    {
        args.push_back(processName);
        args.push_back(util::strformat("--fds-root=%s", fds_root.c_str()));
        args.push_back(util::strformat("--fds.pm.platform_uuid=%ld", platformUuid));
        args.push_back(util::strformat("--fds.pm.platform_port=%d", platformPort));
        start();
    }
    ~ProcessHandle()
    {
        stop();
    }
    void start()
    {
        proc.reset(new T((int) args.size(), (char**) &(args[0]), true));
        thread.reset(new std::thread([&]() { proc->main(); }));
        proc->getReadyWaiter().await();
    }
    void stop()
    {
        if (proc) { proc->stop(); }
        if (thread) { thread->join(); }
        proc.reset(nullptr);
        thread.reset(nullptr);
    }
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

TEST(ProcHandle, DISABLED_test1) {
    ProcessHandle<fds::DmProcess> dm1("DmProce" , "/fds/node1", 2048, 9850);
    dm1.stop();
}

TEST_F(ClusterFixture, test1)
{
    // Temporary hack
    // TODO(Rao): Get rid of this by putting perftraceing under PERF macro
    ProcessHandle<fds::Om>          om("Om", "/fds", 1024, 7000);
    g_fdsprocess = om.proc.get();
    g_cntrs_mgr = om.proc->get_cntrs_mgr();

    ProcessHandle<fds::AmProcess>   am("Om", "/fds", 1024, 7000);
    ProcessHandle<fds::DmProcess>   dm1("DmProcess", "/fds", 1024, 7000);
    ProcessHandle<fds::DmProcess>   dm2("DmProcess", "/fds/node1", 2048, 9850);
    ProcessHandle<fds::DmProcess>   dm3("DmProcess", "/fds/node2", 4096, 10850);

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

    char* args2[] = {"DmProc", "--fds-root=/fds/node1",
                    "--fds.pm.platform_uuid=2048",
                    "--fds.pm.platform_port=9850"};
    dmProc2.reset(new fds::DmProcess(4, args2, true));
    std::thread t4([&] { dmProc2->main(); });
    dmProc2->getReadyWaiter().await();

    char* args3[] = {"DmProc", "--fds-root=/fds/node2",
                    "--fds.pm.platform_uuid=4096",
                    "--fds.pm.platform_port=10850"};
    dmProc3.reset(new fds::DmProcess(4, args3, true));
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

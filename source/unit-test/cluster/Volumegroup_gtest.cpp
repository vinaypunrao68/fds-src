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

struct ClusterFixture : BaseTestFixture {
    virtual void SetUp() override {
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
    }
    std::unique_ptr<fds::DmProcess> dmProc;
    std::unique_ptr<fds::AmProcess> amProc;
    std::unique_ptr<fds::Om> omProc;
};

// TODO(Rao): Svc map checker

TEST_F(ClusterFixture, basic)
{
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    g_fdslog = new fds_log("volumegtest");
    po::options_description opts("Allowed options");
    ClusterFixture::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

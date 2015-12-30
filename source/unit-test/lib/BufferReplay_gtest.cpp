
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <BufferReplay.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testlib/TestFixtures.h>
#include <testlib/TestUtils.h>
#include <testlib/DataGen.hpp>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

template <class F>
auto wrapper(F &&f) -> decltype(f) {
    return f;
}

struct BufferReplayTest : BaseTestFixture {
    void SetUp() override;
    void progressCb(BufferReplay::Progress status);
    void replayOpsCb(int64_t idx, std::list<BufferReplay::Op>& ops);

    int32_t maxOpsToReplay{4};
    BufferReplay::Progress reportedProgress;
    int32_t opsReplayed {0};
    concurrency::TaskStatus replayCaughtupWaiter;
    concurrency::TaskStatus donewaiter;
    std::unique_ptr<fds_threadpool> tp;
    std::unique_ptr<BufferReplay> br;
    std::unique_ptr<CachedRandDataGenerator<>> stringCache;
};

void BufferReplayTest::SetUp()
{
    tp.reset(new fds_threadpool(2));
    br.reset(new BufferReplay("ops", maxOpsToReplay, tp.get()));
    auto e = br->init();
    ASSERT_TRUE(e == ERR_OK);
    br->setProgressCb(std::bind(&BufferReplayTest::progressCb, this,
                                std::placeholders::_1));
    br->setReplayOpsCb(std::bind(&BufferReplayTest::replayOpsCb, this,
                                 std::placeholders::_1, std::placeholders::_2));
    stringCache.reset(new CachedRandDataGenerator<>(64, true, 16, 16));
}

void BufferReplayTest::progressCb(BufferReplay::Progress status)
{
    reportedProgress = status;
    if (status == BufferReplay::ABORTED ||
        status == BufferReplay::COMPLETE) {
        donewaiter.done();
    } else if (status == BufferReplay::REPLAY_CAUGHTUP) {
        replayCaughtupWaiter.done();
    }
}

void BufferReplayTest::replayOpsCb(int64_t replayIdx, std::list<BufferReplay::Op>& ops)
{
    opsReplayed += ops.size();
    ASSERT_TRUE(static_cast<int32_t>(ops.size()) <= maxOpsToReplay);

    /* Ensure replayed ops are what they are supposed to be */
    for (const auto &op : ops) {
        ASSERT_TRUE(*(op.second) == *(stringCache->itemAt(replayIdx)));
        replayIdx++;
    }

    /* Notify so more ops can be replayed */
    int cnt = ops.size();
    tp->schedule([this, cnt]()  { br->notifyOpsReplayed(cnt); });
}

TEST_F(BufferReplayTest, test1) {
    uint32_t numops = this->getArg<uint32_t>("numops");
    for (uint32_t i = 0; i < numops; i++) {
        Error e = br->buffer(std::make_pair(i, stringCache->itemAt(i)));
        ASSERT_TRUE(e == ERR_OK);
    }
    br->startReplay();
    donewaiter.await();
    ASSERT_TRUE(reportedProgress == BufferReplay::COMPLETE);
    ASSERT_TRUE(opsReplayed == static_cast<int32_t>(numops));
}

TEST_F(BufferReplayTest, reject_after_catchup) {
    uint32_t numOps = 1024;
    for (uint32_t i = 0; i < numOps; i++) {
        Error e = br->buffer(std::make_pair(i, stringCache->itemAt(i)));
        ASSERT_TRUE(e == ERR_OK);
    }

    br->startReplay();

    Error e = ERR_OK;
    for (uint32_t i = 0; i < numOps * 4; i++) {
        std::this_thread::sleep_for(std::chrono::microseconds(2));
        e = br->buffer(std::make_pair(i, stringCache->itemAt(i)));
        if (e != ERR_OK) {
            break;
        }
    }
    ASSERT_TRUE(e == ERR_UNAVAILABLE);
    ASSERT_TRUE(reportedProgress == BufferReplay::REPLAY_CAUGHTUP ||
                reportedProgress == BufferReplay::COMPLETE);
    donewaiter.await();
}


TEST_F(BufferReplayTest, prereplay_abort) {
    Error e = br->buffer(std::make_pair(0, stringCache->itemAt(0)));
    ASSERT_TRUE(e == ERR_OK);

    br->abort();

    e = br->buffer(std::make_pair(1, stringCache->itemAt(1)));
    ASSERT_TRUE(e != ERR_OK);

    br->startReplay();
    donewaiter.await();
    ASSERT_TRUE(reportedProgress == BufferReplay::ABORTED);
}

TEST_F(BufferReplayTest, postreplay_abort) {
    Error e = br->buffer(std::make_pair(0, stringCache->itemAt(0)));
    ASSERT_TRUE(e == ERR_OK);

    br->startReplay();

    br->abort();

    e = br->buffer(std::make_pair(1, stringCache->itemAt(1)));
    ASSERT_TRUE(e != ERR_OK);

    donewaiter.await();
    ASSERT_TRUE(reportedProgress == BufferReplay::ABORTED);
}


int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("numops", po::value<uint32_t>()->default_value(10), "numops");
    BufferReplayTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

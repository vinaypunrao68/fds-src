
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
    uint32_t affinity {1};
    concurrency::TaskStatus replayCaughtupWaiter;
    concurrency::TaskStatus donewaiter;
    std::unique_ptr<fds_threadpool> tp;
    std::unique_ptr<BufferReplay> br;
    std::unique_ptr<CachedRandDataGenerator<>> stringCache;
};

void BufferReplayTest::SetUp()
{
    tp.reset(new fds_threadpool(2));
    br.reset(new BufferReplay("ops", maxOpsToReplay, affinity, tp.get()));
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
        ASSERT_TRUE(*(op.payload) == *(stringCache->itemAt(replayIdx)));
        replayIdx++;
    }

    /* Notify so more ops can be replayed */
    int cnt = ops.size();
    tp->schedule([this, cnt]()  { br->notifyOpsReplayed(cnt); });
}

TEST_F(BufferReplayTest, test1) {
    int32_t numops = this->getArg<int32_t>("numops");
    for (int32_t i = 0; i < numops; i++) {
        Error e = br->buffer(BufferReplay::Op{i, i, stringCache->itemAt(i)});
        ASSERT_TRUE(e == ERR_OK);
    }
    br->startReplay();
    donewaiter.await();
    ASSERT_TRUE(reportedProgress == BufferReplay::COMPLETE);
    ASSERT_TRUE(opsReplayed == static_cast<int32_t>(numops));
}

TEST_F(BufferReplayTest, reject_after_catchup) {
    int32_t numOps = 1024;
    for (int32_t i = 0; i < numOps; i++) {
        Error e = br->buffer(BufferReplay::Op{i, i, stringCache->itemAt(i)});
        ASSERT_TRUE(e == ERR_OK);
    }

    br->startReplay();

    Error e = ERR_OK;
    for (int32_t i = 0; i < numOps * 4; i++) {
        std::this_thread::sleep_for(std::chrono::microseconds(2));
        e = br->buffer(BufferReplay::Op{i, i, stringCache->itemAt(i)});
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
    Error e = br->buffer(BufferReplay::Op{0, 0, stringCache->itemAt(0)});
    ASSERT_TRUE(e == ERR_OK);

    br->abort();

    e = br->buffer(BufferReplay::Op{1, 1, stringCache->itemAt(1)});
    ASSERT_TRUE(e != ERR_OK);

    br->startReplay();
    donewaiter.await();
    ASSERT_TRUE(reportedProgress == BufferReplay::ABORTED);
}

TEST_F(BufferReplayTest, postreplay_abort) {
    Error e = br->buffer(BufferReplay::Op{0, 0, stringCache->itemAt(0)});
    ASSERT_TRUE(e == ERR_OK);

    br->startReplay();

    br->abort();

    e = br->buffer(BufferReplay::Op{1, 1, stringCache->itemAt(1)});
    ASSERT_TRUE(e != ERR_OK);

    donewaiter.await();
    ASSERT_TRUE(reportedProgress == BufferReplay::ABORTED);
}

TEST_F(BufferReplayTest, opsTimeout)
{
    int32_t numops = this->getArg<int32_t>("numops");
    maxOpsToReplay = 512;
    tp.reset(new fds_threadpool(4));
    br.reset(new BufferReplay("ops", maxOpsToReplay, affinity, tp.get()));
    auto e = br->init();
    ASSERT_TRUE(e == ERR_OK);
    br->setProgressCb(std::bind(&BufferReplayTest::progressCb, this,
                                std::placeholders::_1));
    br->setReplayOpsCb([this](int64_t idx, std::list<BufferReplay::Op>& ops) {
        auto replayWork = [this](int64_t idx) {
            int32_t abortidx = this->getArg<int32_t>("abortidx");
            if (idx > abortidx) {
                br->abort();
            }
            br->notifyOpsReplayed();
        };

        bool bUseAffinity = this->getArg<bool>("replayaffinity");
        /* For each op schedule replay work */
        for (const auto &op : ops) {
            if (bUseAffinity) {
                tp->scheduleWithAffinity(affinity, replayWork, idx);
            } else {
                tp->schedule(replayWork, idx);
            }
            idx++;
        }
    });

    /* Buffer some ops */
    for (int32_t i = 0; i < numops; i++) {
        Error e = br->buffer(BufferReplay::Op{i, i, stringCache->itemAt(i)});
        ASSERT_TRUE(e == ERR_OK);
    }

    /* Start replay */
    br->startReplay();

    /* Wait for replay to finish */
    donewaiter.await();
    int32_t abortidx = this->getArg<int32_t>("abortidx");
    ASSERT_TRUE((abortidx >= numops && reportedProgress == BufferReplay::COMPLETE) ||
                reportedProgress == BufferReplay::ABORTED);
    ASSERT_TRUE(br->getOutstandingReplayOpsCnt() == 0);
}


int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("numops", po::value<int32_t>()->default_value(1024), "numops")
        ("abortidx", po::value<int32_t>()->default_value(1024), "abortidx")
        ("replayaffinity", po::value<bool>()->default_value(false), "replayaffinity");
    BufferReplayTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

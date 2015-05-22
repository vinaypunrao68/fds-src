/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <string>
#include <chrono>
#include <cstdlib>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fds_process.h>
#include <MigrationUtility.h>

namespace fds {

static const std::string logname = "sm_seqnum";

TEST(MigrationSeqNum, Sequence1)
{
    MigrationSeqNum seqTest;
    bool completed = false;

    for (int i = 0; i < 10; ++i) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
        EXPECT_FALSE(seqTest.isSeqNumComplete());
    }
    completed = seqTest.setSeqNum(10, true);
    EXPECT_TRUE(completed);
    EXPECT_TRUE(seqTest.isSeqNumComplete());
}

TEST(MigrationSeqNum, Sequence2)
{
    MigrationSeqNum seqTest;
    bool completed = false;

    for (int i = 1; i <= 11; i += 2) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
        EXPECT_FALSE(seqTest.isSeqNumComplete());
    }

    for (int i = 0; i <= 12; i += 2) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
        EXPECT_FALSE(seqTest.isSeqNumComplete());
    }

    completed = seqTest.setSeqNum(13, true);
    EXPECT_TRUE(completed);
    EXPECT_TRUE(seqTest.isSeqNumComplete());
}

TEST(MigrationSeqNum, Sequence3)
{
    MigrationSeqNum seqTest;
    bool completed = false;

    completed = seqTest.setSeqNum(14, true);
    EXPECT_FALSE(completed);
    EXPECT_FALSE(seqTest.isSeqNumComplete());
    for (int i = 13; i > 0; --i) {
        completed = seqTest.setSeqNum(i, false);
    }
    EXPECT_FALSE(completed);
    EXPECT_FALSE(seqTest.isSeqNumComplete());
    completed = seqTest.setSeqNum(0, false);
    EXPECT_TRUE(completed);
    EXPECT_TRUE(seqTest.isSeqNumComplete());
}

TEST(MigrationSeqNum, Sequence4)
{
    MigrationSeqNum seqTest;
    bool completed = false;

    completed = seqTest.setSeqNum(14, true);
    EXPECT_FALSE(completed);
    EXPECT_FALSE(seqTest.isSeqNumComplete());
    for (int i = 0; i < 14; ++i) {
        completed = seqTest.setSeqNum(i, false);
    }
    EXPECT_TRUE(completed);
    EXPECT_TRUE(seqTest.isSeqNumComplete());
}

TEST(MigrationSeqNum, Sequence5)
{
    MigrationSeqNum seqTest;
    bool completed = false;

    completed = seqTest.setSeqNum(14, true);
    EXPECT_FALSE(completed);
    EXPECT_FALSE(seqTest.isSeqNumComplete());
    for (int i = 12; i >= 0; i -= 2) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
        EXPECT_FALSE(seqTest.isSeqNumComplete());
    }
    for (int i = 13; i >= 0; i -= 2) {
        completed = seqTest.setSeqNum(i, false);
    }
    EXPECT_TRUE(completed);
    EXPECT_TRUE(seqTest.isSeqNumComplete());
}

TEST(MigrationSeqNum, Sequence6)
{
    MigrationSeqNum seqTest;
    bool completed = false;

    completed = seqTest.setSeqNum(14, true);
    EXPECT_FALSE(completed);
    EXPECT_FALSE(seqTest.isSeqNumComplete());
    for (int i = 13; i >= 0; i -= 2) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
        EXPECT_FALSE(seqTest.isSeqNumComplete());
    }
    for (int i = 12; i >= 0; i -= 2) {
        completed = seqTest.setSeqNum(i, false);
    }
    EXPECT_TRUE(completed);
    EXPECT_TRUE(seqTest.isSeqNumComplete());
}

TEST(MigrationSeqNum, Sequence7)
{
    MigrationSeqNum seqTest;
    bool completed = false;

    completed = seqTest.setSeqNum(0, true);
    EXPECT_TRUE(completed);
    EXPECT_TRUE(seqTest.isSeqNumComplete());
}

TEST(MigrationSeqNum, Sequence8)
{
    MigrationSeqNum seqTest;
    bool completed = false;

    for (int i = 20; i >=10; --i) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
        EXPECT_FALSE(seqTest.isSeqNumComplete());
    }
    completed = seqTest.setSeqNum(21, true);
    EXPECT_FALSE(completed);
    EXPECT_FALSE(seqTest.isSeqNumComplete());

    completed = seqTest.setSeqNum(0, false);
    EXPECT_FALSE(completed);
    EXPECT_FALSE(seqTest.isSeqNumComplete());

    for (int i = 9; i >= 1; --i) {
        completed = seqTest.setSeqNum(i, false);
    }
    EXPECT_TRUE(completed);
    EXPECT_TRUE(seqTest.isSeqNumComplete());
}

void
randomSequenceTest(MigrationSeqNum& seqTest)
{
    bool completed = false;
    const int maxSeq = 50;
    std::array<int, maxSeq + 1> seqArr;
    for (int i = 0; i <= maxSeq; ++i) {
        seqArr[i] = i;
    }

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    shuffle(seqArr.begin(), seqArr.end(), std::default_random_engine(seed));

#ifdef VERBOSE_OUTPUT
    std::cout << "Sequence: ";
    for (int& x : seqArr) {
        std::cout << ' ' << x;
    }
    std::cout << '\n';
#endif  // VERBOSE_OUTPUT

    bool trueSet = false;
    for (int& x : seqArr) {
        if (x == maxSeq) {
            completed = seqTest.setSeqNum(x, true);
            trueSet = true;
        } else {
            completed = seqTest.setSeqNum(x, false);
            if (!trueSet) {
                EXPECT_FALSE(completed);
            }
        }
    }
    EXPECT_TRUE(completed);
}

TEST(MigrationSeqNum, RandomSequence9)
{
    for (int testIter = 0; testIter < 99; testIter++) {
        MigrationSeqNum seqTest;
        randomSequenceTest(seqTest);
        seqTest.resetSeqNum();
        randomSequenceTest(seqTest);
    }
}

class testTimeout {
  public:
    explicit testTimeout(int id_) {
        id = id_;
        timedOut = false;
    };
    ~testTimeout() {};
    void timeoutHandler()
    {
        timedOut = true;
    };
    uint32_t isTimedOut() {
        return timedOut;
    };

  private:
    uint32_t id;
    bool timedOut;
};

bool stopTestThread = false;

void
thrSeqIncrementForever(MigrationSeqNum &seqNum)
{
    uint64_t num = 0;
    while (!stopTestThread) {
        // std::cout << "num=" << num << std::endl;
        seqNum.setSeqNum(num, false);
        // Sleep a bit before the next iteration.
        sleep(1);
        ++num;
    }
}

void
thrSeqIncrementMax(MigrationSeqNum &seqNum, uint64_t maxLoop, bool setLastSeq)
{
    uint64_t num = 0;
    while (maxLoop > 0) {
        --maxLoop;
        if ((0 == maxLoop) && setLastSeq) {
            // std::cout << "num=" << num << std::endl;
            seqNum.setSeqNum(num, true);
            break;
        } else {
            // std::cout << "num=" << num << std::endl;
            seqNum.setSeqNum(num, false);
        }
        // Sleep a bit before the next iteration.
        sleep(1);
        ++num;
    }
}


//
// setup: two thread forever increasing sequence number and kill it.
//        wait for a while, and timeout should have fired.
// sresult: timeouts
//
TEST(MigrationSeqNum, seqNumTimeout1)
{
    stopTestThread = false;

    FdsTimerPtr timer(new FdsTimer());

    testTimeout to1(111);
    MigrationSeqNum seqTest1(timer, 2, std::bind(&testTimeout::timeoutHandler, &to1));

    testTimeout to2(999);
    MigrationSeqNum seqTest2(timer, 2, std::bind(&testTimeout::timeoutHandler, &to2));

    std::thread thr1(thrSeqIncrementForever, std::ref(seqTest1));
    std::thread thr2(thrSeqIncrementForever, std::ref(seqTest2));

    // Do not remove.
    // Let it run for 20 seconds.
    sleep(10);

    stopTestThread = true;

    thr1.join();
    thr2.join();

    // Do not remove
    // Give time for the timeout handler to fire.
    sleep(5);

    ASSERT_TRUE(to1.isTimedOut());
    ASSERT_TRUE(to2.isTimedOut());

    timer.reset();
    stopTestThread = false;
}

//
// setup: single thread forever increment.  kill thread and wait.
//        timeout routine should fire.
// result:  timeout should trigger.
TEST(MigrationSeqNum, seqNumTimeout2)
{
    stopTestThread = false;

    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationSeqNum seqTest(timer, 2, std::bind(&testTimeout::timeoutHandler, &to));

    std::thread thr(thrSeqIncrementForever, std::ref(seqTest));

    // Do not remove.
    // Let it run for 10 seconds.
    sleep(10);

    stopTestThread = true;
    thr.join();

    // Sleep for a while, so the checker will trigger timeout
    sleep(10);

    ASSERT_TRUE(to.isTimedOut());

    timer.reset();
    stopTestThread = false;
}

// setup: single thread complete sequence and terminate
// result:  timeout should not have fired.
TEST(MigrationSeqNum, seqNumTimeout3)
{
    stopTestThread = false;
    uint64_t maxSeq = 20;
    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationSeqNum seqTest(timer, 2, std::bind(&testTimeout::timeoutHandler, &to));

    std::thread thr(thrSeqIncrementMax, std::ref(seqTest), maxSeq, true);

    thr.join();

    ASSERT_FALSE(to.isTimedOut());

    stopTestThread = false;
}

// setup: single thread complete exactly one time
// result:  timeout should not have fired.
TEST(MigrationSeqNum, seqNumTimeout4)
{
    stopTestThread = false;
    uint64_t maxSeq = 1;
    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationSeqNum seqTest(timer, 2, std::bind(&testTimeout::timeoutHandler, &to));

    std::thread thr(thrSeqIncrementMax, std::ref(seqTest), maxSeq, true);

    thr.join();

    // Do not remove.
    // Sleep is necessary in this test.  T
    sleep(5);

    ASSERT_FALSE(to.isTimedOut());

    stopTestThread = false;
}

// setup: single thread does not complete sequence.
// result:  timeout should have fired.
TEST(MigrationSeqNum, seqNumTimeout5)
{
    stopTestThread = false;
    uint64_t maxSeq = 1;
    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationSeqNum seqTest(timer, 2, std::bind(&testTimeout::timeoutHandler, &to));

    std::thread thr(thrSeqIncrementMax, std::ref(seqTest), maxSeq, false);

    thr.join();

    // Do not remove.
    // Sleep is necessary in this test.  T
    sleep(5);

    ASSERT_TRUE(to.isTimedOut());

    stopTestThread = false;
}

// setup:: two threads: 1) thread loop and never complete sequence.
//                      2) thread complete secuence.
// result: thread 1) should timeout
//         thread 2) should not timeout.
TEST(MigrationSeqNum, seqNumTimeout6)
{
    stopTestThread = false;
    uint64_t maxSeq = 20;
    FdsTimerPtr timer(new FdsTimer());

    testTimeout to1(111);
    MigrationSeqNum seqTest1(timer, 2, std::bind(&testTimeout::timeoutHandler, &to1));

    testTimeout to2(222);
    MigrationSeqNum seqTest2(timer, 2, std::bind(&testTimeout::timeoutHandler, &to1));

    std::thread thr1(thrSeqIncrementForever, std::ref(seqTest1));
    std::thread thr2(thrSeqIncrementMax, std::ref(seqTest2), maxSeq, true);

    thr2.join();
    stopTestThread = true;
    thr1.join();

    // Do not remove.
    // Sleep is necessary in this test.  This tests timeout after certain time.
    sleep(5);

    ASSERT_TRUE(to1.isTimedOut());
    ASSERT_FALSE(to2.isTimedOut());

    stopTestThread = false;
}


}  // namespace fds

int
main(int argc, char *argv[])
{
    srand(time(NULL));
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}


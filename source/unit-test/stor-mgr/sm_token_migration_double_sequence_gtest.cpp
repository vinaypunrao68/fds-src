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

static const std::string logname = "sm_double_seqnum";


TEST(MigrationDoubleSeqNum, Sequence101)
{
    MigrationDoubleSeqNum seqTest;
    bool completed = false;
    uint64_t maxSize1 = 5;
    uint64_t maxSize2 = 2;

    for (uint64_t seqNum1 = 0; seqNum1 <= maxSize1; ++seqNum1) {
        for (uint64_t seqNum2 = 0; seqNum2  <= maxSize2; ++seqNum2) {
            completed = seqTest.setDoubleSeqNum(seqNum1,
                                                (maxSize1 == seqNum1) ? true : false,
                                                seqNum2,
                                                (maxSize2 == seqNum2) ? true : false);

            if ((seqNum1 == maxSize1) && (seqNum2 == maxSize2)) {
                EXPECT_TRUE(completed);
            } else {
                EXPECT_FALSE(completed);
            }
        }
    }
}

TEST(MigrationDoubleSeqNum, Sequence102)
{
    MigrationDoubleSeqNum seqTest;
    bool completed = false;
    uint64_t maxSize1 = 5;
    uint64_t maxSize2 = 2;
    uint64_t seqNum1 = maxSize1;
    uint64_t seqNum2 = maxSize2;

    for (uint64_t i = 0; i <= maxSize1; ++i, --seqNum1) {
        for (uint64_t j = 0; j <= maxSize2; ++j, --seqNum2) {
            completed = seqTest.setDoubleSeqNum(seqNum1,
                                                (maxSize1 == seqNum1) ? true : false,
                                                seqNum2,
                                                (maxSize2 == seqNum2) ? true : false);

            if ((seqNum1 == 0) && (seqNum2 == 0)) {
                EXPECT_TRUE(completed);
                break;
            } else {
                EXPECT_FALSE(completed);
            }
        }
        seqNum2 = maxSize2;
    }
}


void
randomDoubleSequenceTest(MigrationDoubleSeqNum& seqTest)
{
    bool completed = false;
    const int maxSeq = 50;
    int totalSeqNums = 0;
    std::array<int, maxSeq + 1> seqArr1;
    std::array<int, maxSeq + 1> seqArr2;
    for (int i = 0; i <= maxSeq; ++i) {
        seqArr1[i] = i;
        seqArr2[i] = i;
    }

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    shuffle(seqArr1.begin(), seqArr1.end(), std::default_random_engine(seed));
    /* use different seed to shuffle second sequence */
    shuffle(seqArr2.begin(), seqArr2.end(), std::default_random_engine(seed ^ 0xffffffff));

#ifdef VERBOSE_OUTPUT
    std::cout << "Sequence Arr1: ";
    for (int& x : seqArr1) {
        std::cout << ' ' << x;
    }
    std::cout << '\n';

    std::cout << "Sequence Arr2: ";
    for (int& x : seqArr2) {
        std::cout << ' ' << x;
    }
    std::cout << '\n';
#endif  // VERBOSE_OUTPUT

    for (int& seqNum1 : seqArr1) {
        for (int& seqNum2 : seqArr2) {
            completed = seqTest.setDoubleSeqNum(seqNum1,
                                                (maxSeq == seqNum1) ? true : false,
                                                seqNum2,
                                                (maxSeq == seqNum2) ? true : false);
            // std::cout << "completed=" << completed << std::endl;
            if (++totalSeqNums < (maxSeq + 1) * (maxSeq + 1)) {
                EXPECT_FALSE(completed);
            } else {
                EXPECT_TRUE(completed);
            }
        }
    }
    EXPECT_TRUE(completed);

}

TEST(MigrationDoubleSeqNum, RandomDoubleSequence)
{
    for (int testIter = 0; testIter < 99; testIter++) {
        MigrationDoubleSeqNum seqTest;
        randomDoubleSequenceTest(seqTest);
        seqTest.resetDoubleSeqNum();
        randomDoubleSequenceTest(seqTest);
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
        timedOut = true;;
    };
    bool isTimedOut() {
        return timedOut;
    };

  private:
    uint32_t id;
    uint32_t timedOut;
};

bool stopTestThread = false;

void
thrSeqIncrementForever(MigrationDoubleSeqNum &doubleSeqNum)
{
    uint64_t num1 = 0, num2 = 0;
    uint64_t num2max = 3;

    while (!stopTestThread) {
//        std::cout << "num1=" << num1 << ", num2=" << num2 << std::endl;
        doubleSeqNum.setDoubleSeqNum(num1, false, num2, false);
        // Sleep a bit before the next iteration.
        if (num2 >= num2max) {
            ++num1;
            num2 = 0;
        } else {
            ++num2;
        }
         sleep(1);
    }
}


void
thrSeqIncrementMax(MigrationDoubleSeqNum &doubleSeqNum,
                   uint64_t maxNum1,
                   uint64_t maxNum2,
                   bool setLastSeq)
{
    uint64_t num1 = 0, num2 = 0;
    uint64_t iter1 = maxNum1;
    uint64_t iter2 = maxNum2;

    while (iter1-- > 0) {
        while (iter2-- > 0) {
            // std::cout << "num1=" << num1 << ", num2=" << num2
            //           << ", max1=" << maxNum1 << ", max2=" << maxNum2;

            bool num1Last = false;
            bool num2Last = false;
            if (num1 == (maxNum1 - 1)) {
                num1Last = true;
            }
            if (num2 == (maxNum2 - 1)) {
                num2Last = true;
            }

            // std::cout << ", num1Last=" << num1Last
            //           << ", num2Last=" << num2Last
            //           << std::endl;

            doubleSeqNum.setDoubleSeqNum(num1, num1Last, num2, num2Last);

            ++num2;
            sleep(1);
        }
        iter2 = maxNum2;
        ++num1;
        num2 = 0;
    }
}

/**
 * setup: Two threads forever increasing sequence number.  And kill it without
 *        completing the sequence.  wait for a while, and check if the timeout
 *        routine fired or not.
 * result: should time out.
 */
TEST(MigrationDoubleSeqNum, doubleSeqNumTimeout1)
{
    stopTestThread = false;

    FdsTimerPtr timer(new FdsTimer());

    testTimeout to1(111);
    MigrationDoubleSeqNum seqTest1(timer, 2, std::bind(&testTimeout::timeoutHandler, &to1));

    testTimeout to2(999);
    MigrationDoubleSeqNum seqTest2(timer, 3, std::bind(&testTimeout::timeoutHandler, &to2));

    std::thread thr1(thrSeqIncrementForever, std::ref(seqTest1));
    std::thread thr2(thrSeqIncrementForever, std::ref(seqTest2));

    // Do not remove.
    // Let it run for 20 seconds.
    sleep(20);

    // kill thread and join on them.
    stopTestThread = true;

    thr1.join();
    thr2.join();

    // Do not remove.
    // Let it sleep for 10 seconds.  The timeout mechanism sh
    sleep(10);

    ASSERT_TRUE(to1.isTimedOut());
    ASSERT_TRUE(to2.isTimedOut());

    timer.reset();
    stopTestThread = false;
}


/**
 * Setup:  single thread forever increase sequence number.  And kill it without
 *         completion.
 * result: timeout should trigger.
 */
TEST(MigrationDoubleSeqNum, seqNumTimeout2)
{
    stopTestThread = false;

    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationDoubleSeqNum seqTest(timer, 2, std::bind(&testTimeout::timeoutHandler, &to));

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

/**
 * setup: thread increments sequence to completion.
 * result: timeout should not fire.
 */
TEST(MigrationDoubleSeqNum, seqNumTimeout3)
{
    stopTestThread = false;
    uint64_t maxNum1 = 5;
    uint64_t maxNum2 = 5;

    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationDoubleSeqNum seqTest(timer, 3, std::bind(&testTimeout::timeoutHandler, &to));

    std::thread thr(thrSeqIncrementMax, std::ref(seqTest), maxNum1, maxNum2, true);

    thr.join();

    ASSERT_FALSE(to.isTimedOut());

    stopTestThread = false;
}

/**
 * setup: thread sets that has only one sequence number (i.e first == last.
 * result: should not timeout.
 */
TEST(MigrationDoubleSeqNum, seqNumTimeout4)
{
    stopTestThread = false;
    uint64_t maxNum1 = 1;
    uint64_t maxNum2 = 1;
    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationDoubleSeqNum seqTest(timer, 2, std::bind(&testTimeout::timeoutHandler, &to));

    std::thread thr(thrSeqIncrementMax, std::ref(seqTest), maxNum1, maxNum2, true);

    thr.join();

    ASSERT_FALSE(to.isTimedOut());

    stopTestThread = false;
}

/**
 * setup: Two threads: 1) forever increasing sequence number
 *                     2) complete sequence.
 * result: 1) should time out.
 *         2) should not timeout.
 */
TEST(MigrationDoubleSeqNum, doubleSeqNumTimeout5)
{
    stopTestThread = false;

    FdsTimerPtr timer(new FdsTimer());
    uint64_t maxNum1 = 3;
    uint64_t maxNum2 = 3;

    testTimeout to1(111);
    MigrationDoubleSeqNum seqTest1(timer, 2, std::bind(&testTimeout::timeoutHandler, &to1));

    testTimeout to2(999);
    MigrationDoubleSeqNum seqTest2(timer, 2, std::bind(&testTimeout::timeoutHandler, &to2));

    std::thread thr1(thrSeqIncrementForever, std::ref(seqTest1));
    std::thread thr2(thrSeqIncrementMax, std::ref(seqTest2), maxNum1, maxNum2, true);

    // Do not remove.
    // Let it run for 20 seconds.
    sleep(20);

    // kill thread and join on them.
    stopTestThread = true;

    thr1.join();
    thr2.join();

    // Do not remove.
    // Let it sleep for 10 seconds.
    sleep(10);

    ASSERT_TRUE(to1.isTimedOut());
    ASSERT_FALSE(to2.isTimedOut());

    timer.reset();
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


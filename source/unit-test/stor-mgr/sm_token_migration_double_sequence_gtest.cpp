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
    Error error;
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
                   uint64_t Num1,
                   uint64_t Num2,
                   bool setLastSeq)
{
    uint64_t num1 = 0, num2 = 0;
    uint64_t maxNum1 = Num1;
    uint64_t maxNum2 = Num2;

    while (maxNum1 > 0) {
        while (maxNum2 > 0) {
//            std::cout << "num1=" << num1 << ", num2=" << num2
//                      << "max1=" << maxNum1 << ", max2=" << maxNum2
//                      << std::endl;

            if ((maxNum1 == 1) && (maxNum2 == 1)) {
                doubleSeqNum.setDoubleSeqNum(num1, true, num2, true);
            } else {
                doubleSeqNum.setDoubleSeqNum(num1, false, num2, false);
            }

            --maxNum2;
            ++num2;
            sleep(1);
        }

        --maxNum1;
        ++num1;
        num2 = 0;
        maxNum2 = Num2;
    }
}

TEST(MigrationDoubleSeqNum, doubleSeqNumTimeout1)
{
    stopTestThread = false;

    FdsTimerPtr timer(new FdsTimer());

    testTimeout to1(111);
    MigrationDoubleSeqNum seqTest1(timer, 2, std::bind(&testTimeout::timeoutHandler, &to1));
    seqTest1.startProgressCheck();

    testTimeout to2(999);
    MigrationDoubleSeqNum seqTest2(timer, 3, std::bind(&testTimeout::timeoutHandler, &to2));
    seqTest2.startProgressCheck();

    std::thread thr1(thrSeqIncrementForever, std::ref(seqTest1));
    std::thread thr2(thrSeqIncrementForever, std::ref(seqTest2));

    // Do not remove.
    // Let it run for 10 seconds.
    sleep(10);

    // Stop progress check for all sequences.
    seqTest1.stopProgressCheck();
    seqTest2.stopProgressCheck();

    stopTestThread = true;

    thr1.join();
    thr2.join();

    ASSERT_FALSE(to1.isTimedOut());
    ASSERT_FALSE(to2.isTimedOut());

    timer.reset();
    stopTestThread = false;
}

TEST(MigrationDoubleSeqNum, seqNumTimeout2)
{
    stopTestThread = false;

    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationDoubleSeqNum seqTest(timer, 3, std::bind(&testTimeout::timeoutHandler, &to));
    seqTest.startProgressCheck();

    std::thread thr(thrSeqIncrementForever, std::ref(seqTest));

    // Do not remove.
    // Let it run for 10 seconds.
    sleep(10);

    stopTestThread = true;
    thr.join();

    // Sleep for a while, so the checker will trigger timeout
    sleep(10);
    seqTest.stopProgressCheck();

    // Since the sequence progress checker is still running, and the thread
    // that's incrementing the sequence is already terminated, the
    // timeout routine should've fired once.
    ASSERT_TRUE(to.isTimedOut());

    timer.reset();
    stopTestThread = false;
}

TEST(MigrationDoubleSeqNum, seqNumTimeout3)
{
    stopTestThread = false;
    uint64_t maxNum1 = 5;
    uint64_t maxNum2 = 5;

    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationDoubleSeqNum seqTest(timer, 3, std::bind(&testTimeout::timeoutHandler, &to));
    seqTest.startProgressCheck();

    std::thread thr(thrSeqIncrementMax, std::ref(seqTest), maxNum1, maxNum2, true);

    thr.join();

    ASSERT_FALSE(to.isTimedOut());

    stopTestThread = false;
}

TEST(MigrationDoubleSeqNum, seqNumTimeout4)
{
    stopTestThread = false;
    uint64_t maxNum1 = 5;
    uint64_t maxNum2 = 5;
    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationDoubleSeqNum seqTest(timer, 2, std::bind(&testTimeout::timeoutHandler, &to));
    seqTest.startProgressCheck();

    std::thread thr(thrSeqIncrementMax, std::ref(seqTest), maxNum1, maxNum2, true);

    thr.join();

    // Do not remove.
    // Sleep is necessary in this test.  This tests timeout after certain time.
    sleep(5);

    ASSERT_TRUE(to.isTimedOut());

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


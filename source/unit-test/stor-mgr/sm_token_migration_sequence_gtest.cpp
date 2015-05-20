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
        count = 0;
    };
    ~testTimeout() {};
    void timeoutHandler()
    {
        ++count;
    };
    uint32_t getCount() {
        return count;
    };

  private:
    uint32_t id;
    uint32_t count;
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
            seqNum.stopProgressCheck();
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

void
thrSeqIncrementMax2(MigrationSeqNum &seqNum, uint64_t maxLoop, bool setLastSeq)
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
// Setup two sequence number with common timer.
// Two thread updating respective sequence number object.
// At the end of the test, when the progress check is stopped with the
// sequence number progressing, the timeout count should be 0.
//
TEST(MigrationSeqNum, seqNumTimeout1)
{
    stopTestThread = false;

    FdsTimerPtr timer(new FdsTimer());

    testTimeout to1(111);
    MigrationSeqNum seqTest1(timer, 2, std::bind(&testTimeout::timeoutHandler, &to1));
    seqTest1.startProgressCheck();

    testTimeout to2(999);
    MigrationSeqNum seqTest2(timer, 3, std::bind(&testTimeout::timeoutHandler, &to2));
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

    ASSERT_EQ(0, to1.getCount());
    ASSERT_EQ(0, to2.getCount());

    timer.reset();
    stopTestThread = false;
}

//
// This test sets up a timer to monitor the progress of the sequence number.
// And another thread increments the sequence number for a while and terminates.
// The progress monitor should catch that the increment haven't moved for a while
// and increment timeout count to exactly 1.
TEST(MigrationSeqNum, seqNumTimeout2)
{
    stopTestThread = false;

    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationSeqNum seqTest(timer, 3, std::bind(&testTimeout::timeoutHandler, &to));
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
    ASSERT_EQ(1, to.getCount());

    timer.reset();
    stopTestThread = false;
}


TEST(MigrationSeqNum, seqNumTimeout3)
{
    stopTestThread = false;
    uint64_t maxSeq = 20;
    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationSeqNum seqTest(timer, 3, std::bind(&testTimeout::timeoutHandler, &to));
    seqTest.startProgressCheck();

    std::thread thr(thrSeqIncrementMax, std::ref(seqTest), maxSeq, true);

    thr.join();

    ASSERT_EQ(0, to.getCount());

    stopTestThread = false;
}

TEST(MigrationSeqNum, seqNumTimeout4)
{
    stopTestThread = false;
    uint64_t maxSeq = 20;
    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationSeqNum seqTest(timer, 2, std::bind(&testTimeout::timeoutHandler, &to));
    seqTest.startProgressCheck();

    std::thread thr(thrSeqIncrementMax, std::ref(seqTest), maxSeq, false);

    thr.join();

    // Do not remove.
    // Sleep is necessary in this test.  This tests timeout after certain time.
    sleep(5);

    ASSERT_EQ(1, to.getCount());

    stopTestThread = false;
}

TEST(MigrationSeqNum, seqNumTimeout5)
{
    stopTestThread = false;
    uint64_t maxSeq = 20;
    FdsTimerPtr timer(new FdsTimer());

    testTimeout to(111);
    MigrationSeqNum seqTest(timer, 2, std::bind(&testTimeout::timeoutHandler, &to));

    std::thread thr(thrSeqIncrementMax2, std::ref(seqTest), maxSeq, true);

    thr.join();

    // Do not remove.
    // Sleep is necessary in this test.  This tests timeout after certain time.
    sleep(5);

    ASSERT_EQ(0, to.getCount());

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


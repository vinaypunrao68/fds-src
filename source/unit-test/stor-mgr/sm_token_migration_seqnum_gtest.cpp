/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fds_process.h>
#include <MigrationUtility.h>


namespace fds {

static const std::string logname = "sm_seqnum";

TEST(MigrationSeqNumReceiver, Sequence1)
{
    MigrationSeqNumReceiver seqTest;
    bool completed = false;

    for (int i = 0; i < 10; ++i) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
    }
    completed = seqTest.setSeqNum(10, true);
    EXPECT_TRUE(completed);
}

TEST(MigrationSeqNumReceiver, Sequence2)
{
    MigrationSeqNumReceiver seqTest;
    bool completed = false;

    for (int i = 1; i <= 11; i += 2) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
    }

    for (int i = 0; i <= 12; i += 2) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
    }

    completed = seqTest.setSeqNum(13, true);
    EXPECT_TRUE(completed);
}

TEST(MigrationSeqNumReceiver, Sequence3)
{
    MigrationSeqNumReceiver seqTest;
    bool completed = false;

    completed = seqTest.setSeqNum(14, true);
    EXPECT_FALSE(completed);
    for (int i = 13; i > 0; --i) {
        completed = seqTest.setSeqNum(i, false);
    }
    EXPECT_FALSE(completed);
    completed = seqTest.setSeqNum(0, false);
    EXPECT_TRUE(completed);
}

TEST(MigrationSeqNumReceiver, Sequence4)
{
    MigrationSeqNumReceiver seqTest;
    bool completed = false;

    completed = seqTest.setSeqNum(14, true);
    EXPECT_FALSE(completed);
    for (int i = 0; i < 14; ++i) {
        completed = seqTest.setSeqNum(i, false);
    }
    EXPECT_TRUE(completed);
}

TEST(MigrationSeqNumReceiver, Sequence5)
{
    MigrationSeqNumReceiver seqTest;
    bool completed = false;

    completed = seqTest.setSeqNum(14, true);
    EXPECT_FALSE(completed);
    for (int i = 12; i >= 0; i -= 2) {
        completed = seqTest.setSeqNum(i, false);
    }
    for (int i = 13; i >= 0; i -= 2) {
        completed = seqTest.setSeqNum(i, false);
    }
    EXPECT_TRUE(completed);
}

TEST(MigrationSeqNumReceiver, Sequence6)
{
    MigrationSeqNumReceiver seqTest;
    bool completed = false;

    completed = seqTest.setSeqNum(14, true);
    EXPECT_FALSE(completed);
    for (int i = 13; i >= 0; i -= 2) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
    }
    for (int i = 12; i >= 0; i -= 2) {
        completed = seqTest.setSeqNum(i, false);
    }
    EXPECT_TRUE(completed);
}

TEST(MigrationSeqNumReceiver, Sequence7)
{
    MigrationSeqNumReceiver seqTest;
    bool completed = false;

    completed = seqTest.setSeqNum(0, true);
    EXPECT_TRUE(completed);
}

TEST(MigrationSeqNumReceiver, Sequence8)
{
    MigrationSeqNumReceiver seqTest;
    bool completed = false;

    for (int i = 20; i >=10; --i) {
        completed = seqTest.setSeqNum(i, false);
        EXPECT_FALSE(completed);
    }
    completed = seqTest.setSeqNum(21, true);
    EXPECT_FALSE(completed);

    completed = seqTest.setSeqNum(0, false);
    EXPECT_FALSE(completed);

    for (int i = 9; i >= 1; --i) {
        completed = seqTest.setSeqNum(i, false);
    }
    EXPECT_TRUE(completed);
}

void
randomSequenceTest()
{
    MigrationSeqNumReceiver seqTest;
    bool completed = false;
    const int maxSeq = 100;
    std::array<int, maxSeq + 1> seqArr;
    for (int i = 0; i <= maxSeq; ++i) {
        seqArr[i] = i;
    }

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    shuffle(seqArr.begin(), seqArr.end(), std::default_random_engine(seed));

    std::cout << "Sequence: ";
    for (int& x : seqArr) {
        std::cout << ' ' << x;
    }
    std::cout << '\n';

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

TEST(MigrationSeqNumReceiver, RandomSequence9)
{
    for (int testIter = 0; testIter < 9999; testIter++) {
        randomSequenceTest();
    }
}


}  // namespace fds

int
main(int argc, char *argv[])
{
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}


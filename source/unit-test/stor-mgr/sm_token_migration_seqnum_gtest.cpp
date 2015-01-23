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
randomSequenceTest()
{
    MigrationSeqNum seqTest;
    bool completed = false;
    const int maxSeq = 50;
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

TEST(MigrationSeqNum, RandomSequence9)
{
    for (int testIter = 0; testIter < 99; testIter++) {
        randomSequenceTest();
    }
}

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
randomDoubleSequenceTest()
{
    MigrationDoubleSeqNum seqTest;
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
        randomDoubleSequenceTest();
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


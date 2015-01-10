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


}  // namespace fds

int
main(int argc, char *argv[])
{
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}


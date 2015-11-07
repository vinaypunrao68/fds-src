/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <string>
#include <iostream>

#include <util/timeutils.h>
#include <util/path.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace fds;  // NOLINT
struct UtilTest : ::testing::Test {
    virtual void SetUp() override {
    }

    virtual void TearDown() override {
    }
};

TEST_F(UtilTest, time) {

    EXPECT_EQ(1, util::getSecondsFromHumanTime("1"));
    EXPECT_EQ(2, util::getSecondsFromHumanTime("2s"));
    EXPECT_EQ(2, util::getSecondsFromHumanTime("2 s"));
    EXPECT_EQ(2, util::getSecondsFromHumanTime("2 sec"));
    EXPECT_EQ(2, util::getSecondsFromHumanTime("2 Sec"));

    EXPECT_EQ(60, util::getSecondsFromHumanTime("1m"));
    EXPECT_EQ(3600, util::getSecondsFromHumanTime("1h"));
    EXPECT_EQ(2*86400, util::getSecondsFromHumanTime("2d"));

    EXPECT_EQ(30, util::getSecondsFromHumanTime("30"));
    EXPECT_EQ(3600 + 24*60 + 30, util::getSecondsFromHumanTime("1:24:30"));
    EXPECT_EQ(210, util::getSecondsFromHumanTime("3:30"));
}

TEST_F(UtilTest, checksum) {
    std::cout << "chksum of /tmp/Log.cpp : " << util::getFileChecksum("/tmp/Log.cpp");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

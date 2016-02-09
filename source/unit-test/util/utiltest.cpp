/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <string>
#include <iostream>
#include <fds_defines.h>
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

    EXPECT_EQ(0, util::getSecondsFromHumanTime(""));
    EXPECT_EQ(0, util::getSecondsFromHumanTime("   "));

    EXPECT_EQ(1, util::getSecondsFromHumanTime("1"));
    EXPECT_EQ(2, util::getSecondsFromHumanTime("2s"));
    EXPECT_EQ(2, util::getSecondsFromHumanTime("2 s"));
    EXPECT_EQ(2, util::getSecondsFromHumanTime("2 sec"));
    EXPECT_EQ(2, util::getSecondsFromHumanTime("2 Sec"));

    EXPECT_EQ(60, util::getSecondsFromHumanTime("1m"));
    EXPECT_EQ(3600, util::getSecondsFromHumanTime("1 h"));
    EXPECT_EQ(2*86400, util::getSecondsFromHumanTime("2d"));

    EXPECT_EQ(30, util::getSecondsFromHumanTime("30"));
    EXPECT_EQ(3600 + 24*60 + 30, util::getSecondsFromHumanTime("1:24:30"));
    EXPECT_EQ(210, util::getSecondsFromHumanTime("3:30"));
}

TEST_F(UtilTest, size) {

    EXPECT_EQ(0, util::getBytesFromHumanSize(""));
    EXPECT_EQ(0, util::getBytesFromHumanSize("  "));

    EXPECT_EQ(1, util::getBytesFromHumanSize("1"));
    EXPECT_EQ(2, util::getBytesFromHumanSize("2B"));
    EXPECT_EQ(2, util::getBytesFromHumanSize("2 B"));
    EXPECT_EQ(2, util::getBytesFromHumanSize("2 bytes"));
    EXPECT_EQ(2, util::getBytesFromHumanSize("2 Bytes"));

    EXPECT_EQ(1024, util::getBytesFromHumanSize("1K"));
    EXPECT_EQ(1024, util::getBytesFromHumanSize("1 K"));
    EXPECT_EQ(1024, util::getBytesFromHumanSize("1KB"));
    EXPECT_EQ(1024, util::getBytesFromHumanSize("1 kb"));

    EXPECT_EQ(3*MB, util::getBytesFromHumanSize("3 MB"));
    EXPECT_EQ(3*MB, util::getBytesFromHumanSize("3 M"));
    EXPECT_EQ(3*MB, util::getBytesFromHumanSize("3M"));
    
    EXPECT_EQ(4L*GB, util::getBytesFromHumanSize("4GB"));
    EXPECT_EQ(4L*GB, util::getBytesFromHumanSize("4G"));
}

TEST_F(UtilTest, path) {
    std::vector<std::string> paths;
    util::getPATH(paths);
    for (const auto& item : paths) {
        std::cout << item << std::endl;
    }
    std::string testpath="/fds/bin_test";
    util::addPATH(testpath);
    paths.clear();
    util::getPATH(paths);
    bool fExists = false;
    for (const auto& item : paths) {
        if (item == testpath) {
            fExists = true;
            break;
        }
    }

    EXPECT_EQ(true, fExists);
    util::removePATH(testpath);
    paths.clear();
    util::getPATH(paths);
    fExists = false;
    for (const auto& item : paths) {
        if (item == testpath) {
            fExists = true;
            break;
        }
    }
    EXPECT_EQ(0, fExists);
}

TEST_F(UtilTest, which) {
    std::string binary = util::which("gzip");
    EXPECT_EQ("/bin/gzip", binary);
    binary = util::which("gzip___test");
    EXPECT_EQ("", binary);
}

TEST_F(UtilTest, checksum) {
    // std::cout << "chksum of /tmp/Log.cpp : " << util::getFileChecksum("/tmp/Log.cpp");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

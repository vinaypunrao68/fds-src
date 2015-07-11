/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <string>
#include <iostream>

#include <util/bloomfilter.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace fds;  // NOLINT
struct BFTest : ::testing::Test {
    virtual void SetUp() override {
    }

    virtual void TearDown() override {
    }
};

TEST_F(BFTest, basic) {
    fds::util::BloomFilter bf;

    std::vector<uint32_t> positions;
    for(const auto&pos : bf.generatePositions("test1",5)) { std::cout << pos << " ";} std::cout << std::endl;
    for(const auto&pos : bf.generatePositions("test3",5)) { std::cout << pos << " ";} std::cout << std::endl;
    for(const auto&pos : bf.generatePositions("test4",5)) { std::cout << pos << " ";} std::cout << std::endl;
    for(const auto&pos : bf.generatePositions("test5",5)) { std::cout << pos << " ";} std::cout << std::endl;
    
    bf.add("test1");
    bf.add("test3");
    bf.add("test4");

    EXPECT_TRUE(bf.lookup("test1"));
    EXPECT_FALSE(bf.lookup("test2"));
    EXPECT_TRUE(bf.lookup("test3"));
    EXPECT_TRUE(bf.lookup("test4"));
    EXPECT_FALSE(bf.lookup("test5"));

    std::string bfFileName("bftest.bf");
    // save the bloom
    serialize::Serializer* s=serialize::getFileSerializer(bfFileName);
    if (bf.write(s) <= 0) {
        std::cout << "read failed" << std::endl;
    }
    delete s;

    fds::util::BloomFilter bf1;
    serialize::Deserializer* d=serialize::getFileDeserializer(bfFileName);
    if (bf1.read(d) <=0) {
        std::cout << "read failed" << std::endl;
    }

    EXPECT_TRUE(bf1.lookup("test1"));
    EXPECT_FALSE(bf1.lookup("test2"));
    EXPECT_TRUE(bf1.lookup("test3"));
    EXPECT_TRUE(bf1.lookup("test4"));
    EXPECT_FALSE(bf1.lookup("test5"));

}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

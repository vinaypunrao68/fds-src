/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <BitSet.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;

using namespace fds;  // NOLINT

TEST(BitSet, basic1)
{
    bool result;
    BitSet *bitset = new BitSet(129);

    result = bitset->set((size_t)63);
    EXPECT_TRUE(result);
    EXPECT_TRUE(bitset->test((size_t)63));

    std::cout << bitset;

    for (size_t i = 0; i < 129; ++i) {
        if (i == 63) {
            EXPECT_TRUE(bitset->test(i));
        } else {
            EXPECT_FALSE(bitset->test(i));
        }
    }

    result = bitset->reset((size_t)63);
    EXPECT_TRUE(result);
    EXPECT_FALSE(bitset->test((size_t)63));

    delete bitset;

}

TEST(BitSet, DISABLED_basic2)
{
    bool result;
    BitSet *bitset = new BitSet(129);
    for (size_t i = 0; i < 129; ++i) {
        EXPECT_TRUE(bitset->set(i));
    }

    for (size_t i = 0; i < 129; ++i) {
        EXPECT_TRUE(bitset->test(i));
    }
    delete bitset;
}


int
main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

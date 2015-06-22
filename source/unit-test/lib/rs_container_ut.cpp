/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <fds_resource.h>
#include <gtest/gtest.h>
#include <testlib/TestFixtures.h>
#include <gmock/gmock.h>
#include <sstream>
#include <random>

using namespace ::fds;

class TestResource : public Resource {
  public:
    explicit TestResource(const ResourceUUID &uuid) : Resource(uuid) {}
    ~TestResource() {}
};

class TestIter : public ResourceIter {

  public:
    TestIter() : ResourceIter(), total_iters(0) {}
    bool rs_iter_fn(Resource::pointer ptr) {
        total_iters++;
        return true;
    }
    fds_uint32_t total_iters;
};

class TestContainer : public RsContainer {
  public:
    TestContainer() {}
    ~TestContainer() {}

    /**
     * Convenience function to create a resource, and set it's name to an int value
     */
    Resource::pointer makeResource(int i) {
        Resource::pointer res = this->rs_alloc_new(i);
        std::string name;
        std::stringstream stringstream;
        stringstream << i;
        stringstream >> name;
        res->setName(name.c_str());
        return res;
    }

  protected:
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new TestResource(uuid);
    }
};

class RsContainerTest : public ::testing::Test {

  public:
    RsContainerTest() : testContainer() {}
    ~RsContainerTest() {}

  protected:
    TestContainer testContainer;
};

/**
 * Test to ensure that rs_alloc_new works properly
 */
TEST_F(RsContainerTest, rs_alloc_new_test) {
    Resource::pointer rp = nullptr;
    rp = testContainer.rs_alloc_new(1L);
    ASSERT_TRUE(rp != nullptr);
}

/**
 * Test to ensure that rs_register works properly; covers rs_register_mtx implicitly
 */
TEST_F(RsContainerTest, rs_register) {
    auto res = testContainer.makeResource(1);
    auto ret = testContainer.rs_register(res);
    ASSERT_TRUE(ret == ERR_OK);
}

/**
 * Test to ensure that we've got proper bounds checking in place
 */
TEST_F(RsContainerTest, bounds_test) {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(RS_DEFAULT_ELEM_CNT + 1, RS_DEFAULT_ELEM_CNT * 2);
    int end_point = distribution(generator);

    long i = 1;
    while(i < end_point) {
        auto res = testContainer.makeResource(i);
        auto ret = testContainer.rs_register(res);
        if (ret == ERR_OUT_OF_MEMORY) {
            break;
        }
        ++i;
    }
    // Error will occur on RS_DEFAULT_ELEM_COUNT + 1
    ASSERT_TRUE(i == (RS_DEFAULT_ELEM_CNT + 1));
}

/**
 * Test to ensure that rs_available_elm() method is correct
 */
TEST_F(RsContainerTest, available_elm) {

    ASSERT_TRUE(testContainer.rs_available_elm() == 0);

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(1, RS_DEFAULT_ELEM_CNT - 1);
    uint end_point = distribution(generator);

    for (uint i = 1; i < end_point; ++i) {
        auto res = testContainer.makeResource(i);
        auto ret = testContainer.rs_register(res);
    }

    ASSERT_TRUE(testContainer.rs_available_elm() == (end_point - 1));
}

/**
 * Test rs_get_resource (both variants)
 */
TEST_F(RsContainerTest, get_resource) {
    auto res = testContainer.makeResource(1);
    auto ret = testContainer.rs_register(res);

    Resource::pointer testPtr = testContainer.rs_get_resource(1);
    ASSERT_TRUE(testPtr == res);

    Resource::pointer testPtr2 = testContainer.rs_get_resource("1");
    ASSERT_TRUE(testPtr2 == res);
}

/**
 * Test rs_unregister (tests rs_unregister_mtx implicitly)
 */
TEST_F(RsContainerTest, unregister) {

    auto res = testContainer.makeResource(1);
    auto ret = testContainer.rs_register(res);

    ASSERT_TRUE(ret == ERR_OK);

    auto was_success = testContainer.rs_unregister(res);

    ASSERT_TRUE(was_success);
}

/**
 * Tests rs_free_resource
 */
TEST_F(RsContainerTest, free_resource) {
    auto res = testContainer.makeResource(1);
    ASSERT_TRUE(res != nullptr);
    auto added = testContainer.rs_register(res);
    ASSERT_TRUE(added == ERR_OK);
    auto ret = testContainer.rs_free_resource(res);
    ASSERT_TRUE(ret);
}

/**
 * Test the rs_foreach
 */
TEST_F(RsContainerTest, rs_foreach) {

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(1, RS_DEFAULT_ELEM_CNT - 1);
    uint end_point = distribution(generator);

    for(uint i = 1; i < end_point; ++i) {
        auto res = testContainer.makeResource(i);
        auto ret = testContainer.rs_register(res);
        ASSERT_TRUE(ret == ERR_OK);
    }

    TestIter * iter = new TestIter;
    testContainer.rs_foreach(iter);
    ASSERT_TRUE(iter->total_iters == (end_point - 1));
}

/**
 * Test the snapshot functionality
 */
TEST_F(RsContainerTest, rs_snapshot) {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(1, RS_DEFAULT_ELEM_CNT - 1);
    uint end_point = distribution(generator);

    for(uint i = 1; i < end_point; ++i) {
        auto res = testContainer.makeResource(i);
        auto ret = testContainer.rs_register(res);
        ASSERT_TRUE(ret == ERR_OK);
    }

    RsArray * clone = new RsArray;
    testContainer.rs_container_snapshot(clone);

    for(uint i = 1; i < end_point; ++i) {
        ASSERT_TRUE(clone->at(i) == testContainer.rs_get_resource(i));
    }
}

int
main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

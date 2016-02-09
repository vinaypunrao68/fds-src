/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cstdio>
#include <string>
#include <vector>
#include <bitset>
#include <random>
#include "boost/smart_ptr/make_shared.hpp"

#include <fds_types.h>
#include <cache/SharedKvCache.h>
#include <concurrency/ThreadPool.h>

using namespace fds;    // NOLINT

static const fds_uint32_t MAX_TEST_OBJ = 1000;

static fds_uint32_t keyCounter = 0;

static fds_threadpool* pool;
static std::mt19937 twister_32;

struct TestObject {
    fds_uint32_t key;
    std::string value;

    TestObject() : key(keyCounter++),
                   value(std::to_string(twister_32())) {}
};

typedef SharedKvCache<fds_uint32_t, TestObject> cache_manager_type;
cache_manager_type* strCacheManager;

namespace by_val {
void getObj(const TestObject& obj) {
    boost::shared_ptr<TestObject> ptr(nullptr);
    strCacheManager->get(obj.key, ptr);
    if (ptr) {
        fds_assert(ptr->value == obj.value);
    }
}

void addObj(const TestObject& obj) {
    std::cout << "Adding value " << obj.value << " with key " << obj.key <<
        std::dec << std::endl;
    strCacheManager->add(obj.key, obj);
    pool->schedule(getObj, obj);
}
}  // namespace by_val

namespace by_shp {
void getObj(boost::shared_ptr<TestObject> obj) {
    boost::shared_ptr<TestObject> ptr(nullptr);
    strCacheManager->get(obj->key, ptr);
    if (ptr) {
        fds_assert(ptr->value == obj->value);
    }
}

void addObj(boost::shared_ptr<TestObject> obj) {
    std::cout << "Adding shared_value " << obj->value << " with key " << obj->key <<
        std::dec << std::endl;
    strCacheManager->add(obj->key, obj);
    pool->schedule(getObj, obj);
}
}  // namespace by_shp

int
main(int argc, char** argv) {
    SharedKvCache<fds_uint32_t, fds_uint32_t> cacheManager("Integer cache manager", 50);

    fds_uint32_t k1 = 1;
    fds_uint32_t k2 = 2;
    fds_uint32_t k3 = 3;
    fds_uint32_t k4 = 4;
    auto evicted = cacheManager.add(k1, k1);
    if (!evicted) {
        GLOGNORMAL << "Didn't evict anything";
    }

    cacheManager.add(k2, k2);
    cacheManager.add(k3, k3);
    evicted = cacheManager.add(k4, k4);
    if (evicted) {
        GLOGNORMAL << "Evicted.";
    }

    decltype(cacheManager)::value_type getV2;
    fds::Error err = cacheManager.get(k2, getV2);
    fds_verify(err == fds::ERR_OK);
    GLOGNORMAL << "Read out value " << *getV2;

    fds_uint32_t k5 = 5;
    evicted = cacheManager.add(k5, k5);
    if (evicted) {
        GLOGNORMAL << "Evicted.";
    }

    // Test KvCache with by_value semantics
    {
        std::vector<TestObject> testObjs(MAX_TEST_OBJ);
        {
            strCacheManager = new cache_manager_type("TestObject Cache Value", 50);
            pool = new fds_threadpool;
            for (auto i = testObjs.begin(); testObjs.end() != i; ++i)
                pool->schedule(by_val::addObj, *i);
            delete pool;
            delete strCacheManager;
        }
    }

    keyCounter = 0;
    // Test KvCache with by_shared_ptr semantics
    {
        std::vector<boost::shared_ptr<TestObject> > testObjs;
        for (size_t i = 0; i < MAX_TEST_OBJ; ++i) {
            testObjs.push_back(boost::make_shared<TestObject>());
        }

        {
            strCacheManager = new cache_manager_type("TestObject Cache SP", 50);
            pool = new fds_threadpool;
            for (auto i = testObjs.begin(); testObjs.end() != i; ++i)
                pool->schedule(by_shp::addObj, *i);
            delete pool;
            delete strCacheManager;
        }
    }
    return 0;
}

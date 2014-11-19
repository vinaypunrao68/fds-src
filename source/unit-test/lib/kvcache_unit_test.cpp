/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cstdio>
#include <string>
#include <vector>
#include <random>
#include <bitset>

#include <fds_types.h>
#include <cache/VolumeKvCache.h>
#include <concurrency/ThreadPool.h>

using namespace fds;    // NOLINT

static const fds_uint32_t MAX_TEST_OBJ = 1000;
static const fds_uint32_t MAX_VOLUMES = 50;

static fds_uint32_t keyCounter = 0;

static fds_threadpool pool;
static std::mt19937 twister_32;

struct TestObject {
    fds_volid_t volId;
    fds_uint32_t key;
    std::string * value;

    TestObject() : volId(keyCounter % MAX_VOLUMES), key(keyCounter++),
            value(new std::string(std::to_string(twister_32()))) {}
    ~TestObject() {
    }
};

static std::vector<TestObject> testObjs(MAX_TEST_OBJ);

static VolumeCacheManager<fds_uint32_t, std::string> strCacheManager("String cache manager");

static std::bitset<MAX_VOLUMES> volSet;

void getObj(TestObject & obj) {
    std::string *ptr = 0;
    strCacheManager.get(obj.volId, obj.key, &ptr);
    std::cout << "compare " << *obj.value << " = " << *ptr << std::endl;
    fds_assert(*ptr == *obj.value);
}

void addObj(TestObject & obj) {
    std::cout << "Adding value " << *obj.value << " with key " << obj.key << " and address " <<
        std::hex << obj.value << std::dec << std::endl;
    strCacheManager.add(obj.volId, obj.key, obj.value);
    pool.schedule(getObj, obj);
}

void create(TestObject & obj) {
    if (!volSet[obj.volId]) {
        std::cout << "Creating cache for vol " << obj.volId << std::endl;
        strCacheManager.createCache(obj.volId, 50, fds::LRU);
        volSet[obj.volId] = 1;
    }
    pool.schedule(addObj, obj);
}

int
main(int argc, char** argv) {
    VolumeCacheManager<fds_uint32_t, fds_uint32_t> cacheManager("Integer cache manager");
    fds::fds_volid_t volId = 321;
    cacheManager.createCache(volId, 3, fds::LRU);

    fds_uint32_t k1 = 1;
    fds_uint32_t *v1 = new fds_uint32_t(1);
    fds_uint32_t k2 = 2;
    fds_uint32_t *v2 = new fds_uint32_t(2);
    fds_uint32_t k3 = 3;
    fds_uint32_t *v3 = new fds_uint32_t(3);
    fds_uint32_t k4 = 4;
    fds_uint32_t *v4 = new fds_uint32_t(4);
    std::unique_ptr<fds_uint32_t> evictEntry1 = cacheManager.add(volId, k1, v1);
    if (evictEntry1 == NULL) {
        GLOGNORMAL << "Didn't evict anything";
    }

    cacheManager.add(volId, k2, v2);
    cacheManager.add(volId, k3, v3);
    std::unique_ptr<fds_uint32_t> evictEntry4 = cacheManager.add(volId, k4, v4);
    if (evictEntry4 != NULL) {
        GLOGNORMAL << "Evicted " << *evictEntry4;
    }

    fds_uint32_t *getV2 = NULL;
    fds::Error err = cacheManager.get(volId, k2, &getV2);
    fds_verify(err == fds::ERR_OK);
    GLOGNORMAL << "Read out value " << *getV2;

    fds_uint32_t k5 = 5;
    fds_uint32_t *v5 = new fds_uint32_t(5);
    std::unique_ptr<fds_uint32_t> evictEntry5 = cacheManager.add(volId, k5, v5);
    if (evictEntry5 != NULL) {
        GLOGNORMAL << "Evicted " << *evictEntry5;
    }

    cacheManager.deleteCache(volId);

    for (std::vector<TestObject>::iterator i = testObjs.begin(); testObjs.end() != i; ++i) {
        std::cout << "Details volume=" << (*i).volId << " key=" << (*i).key << " value="
                << *(*i).value << std::endl;
        create(*i);
    }

    sleep(-1);

    return 0;
}

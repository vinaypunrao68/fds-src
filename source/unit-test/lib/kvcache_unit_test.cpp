/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cache/KvCache.h>

int
main(int argc, char** argv) {
    fds::LruKvCache<fds_uint32_t, fds_uint32_t> cache("Unit test cache",
                                                      3);

    fds_uint32_t k1 = 1;
    fds_uint32_t *v1 = new fds_uint32_t(1);
    fds_uint32_t k2 = 2;
    fds_uint32_t *v2 = new fds_uint32_t(2);
    fds_uint32_t k3 = 3;
    fds_uint32_t *v3 = new fds_uint32_t(3);
    fds_uint32_t k4 = 4;
    fds_uint32_t *v4 = new fds_uint32_t(4);
    std::unique_ptr<fds_uint32_t> evictEntry1 = cache.add(k1, v1);
    if (evictEntry1 == NULL) {
        GLOGNORMAL << "Didn't evict anything";
    }
    cache.add(k2, v2);
    cache.add(k3, v3);
    std::unique_ptr<fds_uint32_t> evictEntry4 = cache.add(k4, v4);
    if (evictEntry4 != NULL) {
        GLOGNORMAL << "Evicted " << *evictEntry4;
    }

    fds_uint32_t *getV2 = NULL;
    fds::Error err = cache.get(k2, &getV2);
    fds_verify(err == fds::ERR_OK);
    GLOGNORMAL << "Read out value " << *getV2;

    fds_uint32_t k5 = 5;
    fds_uint32_t *v5 = new fds_uint32_t(5);
    std::unique_ptr<fds_uint32_t> evictEntry5 = cache.add(k5, v5);
    if (evictEntry5 != NULL) {
        GLOGNORMAL << "Evicted " << *evictEntry5;
    }

    return 0;
}

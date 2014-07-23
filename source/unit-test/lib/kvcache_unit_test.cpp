/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cache/VolumeKvCache.h>

int
main(int argc, char** argv) {
    fds::VolumeCacheManager<fds_uint32_t, fds_uint32_t> cacheManager("Unit test cache manager");
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

    return 0;
}

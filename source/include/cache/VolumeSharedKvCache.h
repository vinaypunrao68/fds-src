/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_CACHE_VOLUMESHAREDKVCACHE_H_
#define SOURCE_INCLUDE_CACHE_VOLUMESHAREDKVCACHE_H_

#include <string>
#include <utility>
#include <unordered_map>

#include "concurrency/RwLock.h"

#include "cache/SharedKvCache.h"

namespace fds {

/**
 * A generic multi-volume cache manager. The cache
 * manager maintains per-volume cache structures, allows
 * parameters (e.g, size, eviction policy, etc...) to be
 * set per-volume. The manager also provides an interface
 * that allows cache update/query for each volume.
 * The cache manager provides thread safety for creating
 * and deleting caches and provides thread safe interfaces
 * for per-volume cache access.
 */
template <class K, class V, class _Hash = std::hash<K>>
class VolumeSharedCacheManager : public Module, boost::noncopyable {
    typedef K key_type;
    typedef V mapped_type;
    typedef _Hash hash_type;
    typedef SharedKvCache<key_type, mapped_type, hash_type> cache_type;
    typedef typename cache_type::value_type value_type;

    // Associated cache and synchronization structure
    typedef std::pair<fds_rwlock*, cache_type*> cache_pair;

    // Structure for multiplexing many volumes with volume ID
    // based associativity.
    std::unordered_map<fds_volid_t, cache_pair> vol_cache_map;

    // Protects the cache map structure
    fds_rwlock cacheMapRwlock;

 public:
    explicit VolumeSharedCacheManager(const std::string &modName)
        :   Module(modName.c_str()),
            vol_cache_map()
    { }

    ~VolumeSharedCacheManager() { }

    /**
     * Creates a new cache structure that the manager manages.
     * Currently, the max number of entries and eviction type are
     * the only policy parameters.
     * @param[in] volId        Volume ID associated with the cache
     * @param[in] maxEntries   Maximum number of entries in the cache
     *
     * @return Err if the volume already has an associated cache.
     */
    Error createCache(fds_volid_t volId, typename cache_type::size_type maxEntries) {
        static std::string const cacheModName("Cache module for ");

        SCOPEDWRITE(cacheMapRwlock);
        if (vol_cache_map.count(volId) > 0) {
            return ERR_VOL_DUPLICATE;
        }

        fds_rwlock *rwlock = new fds_rwlock();
        cache_type *cache = new cache_type(cacheModName + std::to_string(volId), maxEntries);
        vol_cache_map[volId] = std::make_pair(rwlock, cache);

        return ERR_OK;
    }

    /**
     * Creates a new cache structure that the manager manages.
     * Currently, the max number of entries and eviction type are
     * the only policy parameters.
     * @param[in] volId        Volume ID associated with the cache
     *
     * @return Err if the volume has no associated cache
     */
    Error deleteCache(fds_volid_t volId) {
        SCOPEDWRITE(cacheMapRwlock);
        auto mapIt = vol_cache_map.find(volId);
        if (mapIt == vol_cache_map.end()) {
            return ERR_NOT_FOUND;
        }

        // Free the entire cache structure
        cache_pair cacheEntry = mapIt->second;
        fds_rwlock *rwlock = cacheEntry.first;
        cache_type *cache = cacheEntry.second;
        delete cache;
        delete rwlock;

        // Remove the map entry
        vol_cache_map.erase(mapIt);
        return ERR_OK;
    }

    /**
     * Adds a key-value pair to a volume's cache.
     */
    value_type add(fds_volid_t volId, const key_type &key, const value_type value) {
        SCOPEDREAD(cacheMapRwlock);
        auto mapIt = vol_cache_map.find(volId);
        // TODO(Andrew): For now just panic if the volume
        // isn't in the manager. Fix later, but the interface
        // needs a bit of change then since we need to return
        // an error code
        fds_verify(mapIt != vol_cache_map.end());

        // Write lock the cache for add
        cache_pair cacheEntry = mapIt->second;
        fds_rwlock *cacheRwlock = cacheEntry.first;
        SCOPEDWRITE(*cacheRwlock);

        // Update the cache structure and return
        // whatever was evicted
        cache_type *cache = cacheEntry.second;
        return cache->add(key, value);
    }

    Error clear(fds_volid_t volId) {
        SCOPEDREAD(cacheMapRwlock);
        auto mapIt = vol_cache_map.find(volId);
        if (mapIt == vol_cache_map.end()) {
            return ERR_NOT_FOUND;
        }

        // Write lock the cache for clearing
        cache_pair cacheEntry = mapIt->second;
        fds_rwlock *cacheRwlock = cacheEntry.first;
        SCOPEDWRITE(*cacheRwlock);

        // Clear the entire cache structure
        cache_type *cache = cacheEntry.second;
        cache->clear();
        return ERR_OK;
    }

    Error get(fds_volid_t volId, const key_type &key, value_type& valueOut) {
        SCOPEDREAD(cacheMapRwlock);
        auto mapIt = vol_cache_map.find(volId);
        if (mapIt == vol_cache_map.end()) {
            return ERR_NOT_FOUND;
        }

        // Write lock the cache for get. We write lock
        // for get since the get actually updates the
        // eviction order structures
        cache_pair cacheEntry = mapIt->second;
        fds_rwlock *cacheRwlock = cacheEntry.first;
        SCOPEDWRITE(*cacheRwlock);

        // Get from the cache
        return cacheEntry.second->get(key, valueOut);
    }

    Error remove(fds_volid_t volId, const key_type &key) {
        SCOPEDREAD(cacheMapRwlock);
        auto mapIt = vol_cache_map.find(volId);
        if (mapIt == vol_cache_map.end()) {
            return ERR_NOT_FOUND;
        }

        // Write lock the cache for remove.
        cache_pair cacheEntry = mapIt->second;
        fds_rwlock *cacheRwlock = cacheEntry.first;
        SCOPEDWRITE(*cacheRwlock);

        // Remove from the cache structure
        cacheEntry.second->remove(key);
        return ERR_OK;
    }

    /// Init module
    int  mod_init(SysParams const *const param) {
        return 0;
    }
    /// Start module
    void mod_startup() {
    }
    /// Shutdown module
    void mod_shutdown() {
    }
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_CACHE_VOLUMESHAREDKVCACHE_H_

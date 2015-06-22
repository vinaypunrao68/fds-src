/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_CACHE_VOLUMEKVCACHE_H_
#define SOURCE_INCLUDE_CACHE_VOLUMEKVCACHE_H_

#include <string>
#include <utility>
#include <list>
#include <unordered_map>
#include <cache/KvCache.h>
#include <concurrency/RwLock.h>

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
class VolumeCacheManager : public Module, boost::noncopyable {
  private:
    /// Associated cache and synchronization structure
    typedef std::pair<fds_rwlock*, KvCache<K, V, _Hash>*> CachePair;

    /**
     * Structure for multiplexing many volumes with volume ID
     * based associativity.
     */
    typedef std::unordered_map<fds_volid_t,
                               CachePair> VolumeCacheMap;
    typedef typename VolumeCacheMap::iterator VolCacheMapIt;
    std::unique_ptr<VolumeCacheMap> volCacheMap;

    /// Protects the cache map structure
    fds_rwlock cacheMapRwlock;

  public:
    explicit VolumeCacheManager(const std::string &modName)
            : Module(modName.c_str()) {
        volCacheMap = std::unique_ptr<VolumeCacheMap>(new VolumeCacheMap());
    }
    ~VolumeCacheManager() {
    }

    /**
     * Creates a new cache structure that the manager manages.
     * Currently, the max number of entries and eviction type are
     * the only policy parameters.
     * @param[in] volId        Volume ID associated with the cache
     * @param[in] maxEntries   Maximum number of entries in the cache
     * @param[in] evictionType Eviction method for the cache
     *
     * @return Err if the volume already has an associated cache.
     */
    Error createCache(fds_volid_t volId,
                      fds_uint32_t maxEntries,
                      EvictionType evictionType) {
        SCOPEDWRITE(cacheMapRwlock);
        if (volCacheMap->count(volId) > 0) {
            return ERR_VOL_DUPLICATE;
        }

        KvCache<K, V, _Hash> *cache;
        fds_rwlock *rwlock = new fds_rwlock();
        std::string cacheModName = "Cache module for ";
        cacheModName += std::to_string(volId.get());
        switch (evictionType) {
            case LRU:
                cache = new LruKvCache<K, V, _Hash>(cacheModName, maxEntries);
                break;
            default:
                fds_panic("Unknown eviction type specified!");
        }
        (*volCacheMap)[volId] = CachePair(rwlock, cache);

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
        VolCacheMapIt mapIt = volCacheMap->find(volId);
        if (mapIt == volCacheMap->end()) {
            return ERR_NOT_FOUND;
        }

        // Free the entire cache structure
        CachePair cacheEntry = mapIt->second;
        fds_rwlock *rwlock = cacheEntry.first;
        KvCache<K, V, _Hash> *cache = cacheEntry.second;
        delete cache;
        delete rwlock;

        // Remove the map entry
        volCacheMap->erase(mapIt);
        return ERR_OK;
    }

    /**
     * Adds a key-value pair to a volume's cache.
     */
    std::unique_ptr<V> add(fds_volid_t volId, const K &key, V* value) {
        SCOPEDREAD(cacheMapRwlock);
        VolCacheMapIt mapIt = volCacheMap->find(volId);
        // TODO(Andrew): For now just panic if the volume
        // isn't in the manager. Fix later, but the interface
        // needs a bit of change then since we need to return
        // an error code
        fds_verify(mapIt != volCacheMap->end());

        // Write lock the cache for add
        CachePair cacheEntry = mapIt->second;
        fds_rwlock *cacheRwlock = cacheEntry.first;
        SCOPEDWRITE(*cacheRwlock);

        // Update the cache structure and return
        // whatever was evicted
        KvCache<K, V, _Hash> *cache = cacheEntry.second;
        return cache->add(key, value);
    }

    /**
     * Adds a key-value pair to a volume's cache.
     */
    std::list<std::unique_ptr<V>> addBatch(fds_volid_t volId,
                                           const std::list<std::pair<K, V*>> pairList) {
        std::list<std::unique_ptr<V>> evictList;

        SCOPEDREAD(cacheMapRwlock);
        VolCacheMapIt mapIt = volCacheMap->find(volId);
        // TODO(Andrew): For now just panic if the volume
        // isn't in the manager. Fix later, but the interface
        // needs a bit of change then since we need to return
        // an error code
        fds_verify(mapIt != volCacheMap->end());

        // Write lock the cache for add
        CachePair cacheEntry = mapIt->second;
        fds_rwlock *cacheRwlock = cacheEntry.first;
        SCOPEDWRITE(*cacheRwlock);
        KvCache<K, V, _Hash> *cache = cacheEntry.second;

        // Iterate and update the cache structure and return
        // whatever was evicted
        std::unique_ptr<V> evictPtr;
        for (typename std::list<std::pair<K, V*>>::const_iterator cit = pairList.begin();
             cit != pairList.end();
             cit++) {
            evictPtr = cache->add((*cit).first, (*cit).second);
            if (evictPtr != NULL) {
                // Transfer ptr ownership to the list
                evictList.push_back(
                    std::unique_ptr<V>(evictPtr.release()));
            }
        }

        return evictList;
    }

    Error clear(fds_volid_t volId) {
        SCOPEDREAD(cacheMapRwlock);
        VolCacheMapIt mapIt = volCacheMap->find(volId);
        if (mapIt == volCacheMap->end()) {
            return ERR_NOT_FOUND;
        }

        // Write lock the cache for clearing
        CachePair cacheEntry = mapIt->second;
        fds_rwlock *cacheRwlock = cacheEntry.first;
        SCOPEDWRITE(*cacheRwlock);

        // Clear the entire cache structure
        KvCache<K, V, _Hash> *cache = cacheEntry.second;
        cache->clear();
        return ERR_OK;
    }

    Error get(fds_volid_t volId,
              const K &key,
              V **valueOut) {
        SCOPEDREAD(cacheMapRwlock);
        VolCacheMapIt mapIt = volCacheMap->find(volId);
        if (mapIt == volCacheMap->end()) {
            return ERR_NOT_FOUND;
        }

        // Write lock the cache for get. We write lock
        // for get since the get actually updates the
        // eviction order structures
        CachePair cacheEntry = mapIt->second;
        fds_rwlock *cacheRwlock = cacheEntry.first;
        SCOPEDWRITE(*cacheRwlock);

        // Get from the cache
        KvCache<K, V, _Hash> *cache = cacheEntry.second;
        return cache->get(key, valueOut);
    }

    Error remove(fds_volid_t volId,
                 const K &key) {
        SCOPEDREAD(cacheMapRwlock);
        VolCacheMapIt mapIt = volCacheMap->find(volId);
        if (mapIt == volCacheMap->end()) {
            return ERR_NOT_FOUND;
        }

        // Write lock the cache for remove.
        CachePair cacheEntry = mapIt->second;
        fds_rwlock *cacheRwlock = cacheEntry.first;
        SCOPEDWRITE(*cacheRwlock);

        // Remove from the cache structure
        KvCache<K, V, _Hash> *cache = cacheEntry.second;
        cache->remove(key);
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

#endif  // SOURCE_INCLUDE_CACHE_VOLUMEKVCACHE_H_

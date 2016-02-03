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
template <class K, class V, class _Hash = std::hash<K>, class StrongAssociation = std::false_type>
struct VolumeSharedCacheManager
{
    typedef K key_type;
    typedef V mapped_type;
    typedef _Hash hash_type;
    typedef SharedKvCache<key_type, mapped_type, hash_type, StrongAssociation> cache_type;
    typedef typename cache_type::value_type value_type;

 private:
    // Structure for multiplexing many volumes with volume ID
    // based associativity.
    std::unordered_map<fds_volid_t, cache_type*> vol_cache_map;

    // Protects the cache map structure
    fds_rwlock cacheMapRwlock;

 public:
    VolumeSharedCacheManager() = default;
    VolumeSharedCacheManager(VolumeSharedCacheManager const&) = delete;
    VolumeSharedCacheManager& operator=(VolumeSharedCacheManager const&) = delete;
    ~VolumeSharedCacheManager() = default;

    /**
     * Creates a new cache structure for the given volume.
     * Currently, the max number of entries is the only policy parameter.
     * @param[in] volId        Volume ID associated with the cache
     * @param[in] maxEntries   Maximum number of entries in the cache
     *
     * @return Err if the volume already has an associated cache.
     */
    Error addVolume(fds_volid_t const volId, typename cache_type::size_type maxEntries) {
        static std::string const cacheModName("Cache module for ");

        SCOPEDWRITE(cacheMapRwlock);
        if (vol_cache_map.count(volId) > 0) {
            return ERR_VOL_DUPLICATE;
        }

        vol_cache_map[volId] = new cache_type(cacheModName + std::to_string(volId.get()), maxEntries);

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
    Error removeVolume(fds_volid_t const volId) {
        SCOPEDWRITE(cacheMapRwlock);
        auto mapIt = vol_cache_map.find(volId);
        if (mapIt == vol_cache_map.end()) {
            return ERR_NOT_FOUND;
        }

        // Free the entire cache structure
        delete mapIt->second;

        // Remove the map entry
        vol_cache_map.erase(mapIt);
        return ERR_OK;
    }

    /**
     * Adds a key-value pair to a volume's cache.
     */
    std::pair<bool, value_type> add(fds_volid_t const volId, const key_type &key, const value_type value, const bool dirty = false) {
        static const std::pair<bool, value_type> nil {false, nullptr};

        SCOPEDREAD(cacheMapRwlock);
        auto mapIt = vol_cache_map.find(volId);
        if (vol_cache_map.end() == mapIt) {
            LOGDEBUG << "Failed to find volume: " << volId
                     << " to insert element [" << key << "]";
            return nil;
        }

        return std::make_pair(true, mapIt->second->add(key, value, dirty));
    }

    std::pair<bool, value_type> add_dirty(fds_volid_t const volId, const key_type &key, const value_type value) {
        return add(volId, key, value, true);
    }

    Error clear(fds_volid_t const volId) {
        SCOPEDREAD(cacheMapRwlock);
        auto mapIt = vol_cache_map.find(volId);
        if (mapIt == vol_cache_map.end()) {
            return ERR_NOT_FOUND;
        }

        // Clear the entire cache structure
        mapIt->second->clear();
        return ERR_OK;
    }

    Error get(fds_volid_t const volId, const key_type &key, value_type& valueOut) {
        SCOPEDREAD(cacheMapRwlock);
        auto mapIt = vol_cache_map.find(volId);
        if (mapIt == vol_cache_map.end()) {
            return ERR_NOT_FOUND;
        }

        // Get from the cache
        return mapIt->second->get(key, valueOut);
    }

    Error remove(fds_volid_t const volId, const key_type &key) {
        SCOPEDREAD(cacheMapRwlock);
        auto mapIt = vol_cache_map.find(volId);
        if (mapIt == vol_cache_map.end()) {
            return ERR_NOT_FOUND;
        }

        // Remove from the cache structure
        mapIt->second->remove(key);
        return ERR_OK;
    }

    template<typename UnaryPredicate>
    Error remove_if(fds_volid_t const volId, UnaryPredicate pred) {
        auto mapIt = vol_cache_map.find(volId);
        if (mapIt == vol_cache_map.end()) {
            return ERR_NOT_FOUND;
        }

        // Remove from the cache structure
        mapIt->second->remove_if(pred);
        return ERR_OK;
    }

};

}  // namespace fds

#endif  // SOURCE_INCLUDE_CACHE_VOLUMESHAREDKVCACHE_H_

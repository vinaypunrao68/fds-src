/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_CACHE_KVCACHE_H_
#define SOURCE_INCLUDE_CACHE_KVCACHE_H_

#include <string>
#include <utility>
#include <list>
#include <fds_error.h>
#include <fds_module.h>
#include <util/Log.h>

namespace fds {

/// Types of eviction
enum EvictionType {
    LRU = 0,
};

/**
 * An abstract interface for a generic key-value cache. The cache
 * takes a basic size, either in number of element or bytes, and
 * caches elements using a specified evication algorithm.
 * The cache itself is NOT thread safe.
 */
template<class A, class B, class _HashBase = std::hash<A>>
class KvCache : public Module, boost::noncopyable {
    protected:
    /// Maximum number of entries in the cache
    // TODO(Andrew): Change to bytes at some point
    fds_uint32_t maxEntries;

    /// Current number of entries
    size_t numEntries;

    /// Eviction algorithm being used
    EvictionType evictionType;

  public:
    /**
     * Constructs the cache object but does not init
     * @param[in] modName     Name of this module
     * @param[in] _maxEntries Maximum number of cache entries
     * @param[in] _eType      Type of eviction policy
     *
     * @return none
     */
    KvCache(const std::string &modName,
            fds_uint32_t _maxEntries)
            : Module(modName.c_str()),
              maxEntries(_maxEntries),
              numEntries(0) {
    }
    ~KvCache() {
    }

    /**
     * Removes a key and value from the cache
     *
     * @param[in] key   Key to use for indexing
     *
     * @return none
     */
    virtual void remove(const A &key) = 0;

    /**
     * Adds a key-value pair to the cache.
     * The cache will take ownership of value pointers added to
     * the cache. That is, the pointer cannot be deleted by
     * whoever passes it in. The cache will become responsible for that.
     * If the key already exists, it will be overwritten.
     * If the cache is full, the evicted entry will be returned.
     * Entries that are evicted are returned as unique_ptr so the
     * caller can use it before it gets freed.
     *
     * @param[in] key   Key to use for indexing
     * @param[in] value Associated value
     *
     * @return An auto_ptr to any evicted entry
     */
    virtual std::unique_ptr<B> add(const A &key, B* value) = 0;

    /**
     * Removes all keys and values from the cache
     *
     * @return none
     */
    virtual void clear() = 0;

    /**
     * Returns the value for the assoicated key. When a value
     * is returned, the cache RETAINS ownership of pointer and
     * it cannot be freed by the caller.
     * When a value is return, the entry's access is updated.
     * 
     * @param[in]  key      Key to use for indexing
     * @param[out] valueOut Pointer to value
     * @para[in]   doTouch  Whether to track this access
     *
     * @return ERR_OK if a value is returned, ERR_NOT_FOUND otherwise.
     */
    virtual Error get(const A &key,
                      B **valueOut,
                      fds_bool_t doTouch = true) = 0;

    /**
     * Checks if a key exists in the cache
     * 
     * @param[in]  key  Key to use for indexing
     *
     * @return true if key exists, false otherwise
     */
    virtual fds_bool_t exists(const A &key) const = 0;

    /**
     * Touches a key for the purpose of representing an access.
     *
     * @param[in]  key  Key to use for indexing
     *
     * @return ERR_OK if a value is returned, ERR_NOT_FOUND otherwise.
     */
    virtual Error touch(const A &key) = 0;

    /**
     * Returns the current number of entries in the cache
     *
     * @return number of entries
     */
    size_t getNumEntries() const {
        return numEntries;
    }

    /**
     * Returns the eviction algorithm
     *
     * @return evicition type
     */
    EvictionType getEvictionType() const {
        return evictionType;
    }
};

/**
 * An LRU eviction based key-value cache. Implements
 * the generic key-value interface.
 */
template <class K, class V, class _Hash = std::hash<K>>
        class LruKvCache : public KvCache<K, V, _Hash> {
  private:
    /**
     * Internal key/value_ptr pairing
     */
    typedef std::pair<K, V*> KvPair;

    /**
     * Eviction algorithm list. Drives which entries
     * to evict.
     */
    typedef typename std::list<KvPair> KvEvictionList;
    typedef typename KvEvictionList::iterator KvMapEntry;
    std::unique_ptr<KvEvictionList> evictionList;

    /**
     * Backing data structure that implements key to value
     * associated lookup.
     */
    typedef typename std::unordered_map<K, KvMapEntry, _Hash> KvCacheMap;
    typedef typename KvCacheMap::const_iterator KvMapIterator;
    std::unique_ptr<KvCacheMap> cacheMap;

  public:
    LruKvCache(const std::string &modName,
               fds_uint32_t _maxEntries)
            : KvCache<K, V, _Hash>(modName, _maxEntries) {
        KvCache<K, V, _Hash>::evictionType = LRU;
        evictionList = std::unique_ptr<KvEvictionList>(
            new KvEvictionList());
        cacheMap = std::unique_ptr<KvCacheMap>(
            new KvCacheMap());
    }
    ~LruKvCache() {
    }

    void remove(const K &key) override {
        // Locate the key in the map
        KvMapIterator mapIt = cacheMap->find(key);
        if (mapIt != cacheMap->end()) {
            KvMapEntry cacheEntry = mapIt->second;
            // Free the old value pointer
            // delete cacheEntry->second;
            delete cacheEntry->second;
            // Remove from the cacheMap
            cacheMap->erase(mapIt);
            // Remove from the evictionList
            evictionList->erase(cacheEntry);
            // Decrement the entry count
            --this->numEntries;
        }
    }

    std::unique_ptr<V> add(const K &key, V* value) override {
        // Touch existing entry...
        if (touch(key) == ERR_OK)
            return std::unique_ptr<V>();

        // otherwise add the entry to the front of the eviction
        // list and into the map
        KvPair entry(key, value);
        evictionList->push_front(entry);
        (*cacheMap)[key] = evictionList->begin();

        // Check if anything needs to be evicted
        if (++this->numEntries > this->maxEntries) {
            GLOGTRACE << "Evicting key " << evictionList->back().first;
            V* entryToEvict = evictionList->back().second;
            fds_verify(entryToEvict != NULL);

            // Remove the cache iterator entry from the map
            cacheMap->erase(evictionList->back().first);
            // Remove the entry from the eviction list
            evictionList->pop_back();
            --this->numEntries;
            // Make sure we're at over the limit
            fds_verify(this->numEntries == this->maxEntries);
            return std::unique_ptr<V>(entryToEvict);
        }

        // Return a NULL pointer if nothing was evicted
        return std::unique_ptr<V>();
    }

    void clear() override {
        // Iterator the eviction list and free the entries
        for (KvMapEntry entry = evictionList->begin();
             entry != evictionList->end();
             entry++) {
            delete entry->second;
        }

        // Clear all of the containers
        evictionList->clear();
        cacheMap->clear();
        this->numEntries = 0;
    }

    Error get(const K &key,
              V **valueOut,
              fds_bool_t doTouch = true) override {
        KvMapIterator mapIt = cacheMap->find(key);
        if (mapIt == cacheMap->end()) {
            return ERR_NOT_FOUND;
        }

        // Get the value to return
        KvMapEntry entry = mapIt->second;
        V* value  = entry->second;

        // Touch the entry in the list if we want
        // to track this access
        if (doTouch == true) {
            fds_verify((this->touch(key) == ERR_OK));
        }

        // Set the return value
        *valueOut = value;
        return ERR_OK;
    }

    Error touch(const K &key) override {
        KvMapIterator mapIt = cacheMap->find(key);
        if (mapIt == cacheMap->end()) {
            return ERR_NOT_FOUND;
        }

        evictionList->splice(evictionList->begin(),
                             *evictionList,
                             mapIt->second);

        return ERR_OK;
    }

    fds_bool_t exists(const K &key) const override {
        return (cacheMap->count(key) > 0);
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

#endif  // SOURCE_INCLUDE_CACHE_KVCACHE_H_

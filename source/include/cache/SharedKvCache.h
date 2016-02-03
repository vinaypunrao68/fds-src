/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_CACHE_SHAREDKVCACHE_H_
#define SOURCE_INCLUDE_CACHE_SHAREDKVCACHE_H_

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/make_shared.hpp"

#include "concurrency/RwLock.h"
#include "fds_error.h"
#include "fds_module.h"
#include "util/Log.h"

namespace fds {

// TODO(bszmyd):  Fri 12 Sep 2014 03:15:47 PM MDT
//   Eviction policy should be a templatization on cache,
//   not an inheritence like KvCache does it. So it should read
//   something like:
//
//   SharedKvCache<int, int, fds::eviction_policy::lru> cache;
//
//   Right now all SharedKvCache's are LRU eviction

/**
 * An abstract interface for a generic key-value cache. The cache
 * takes a basic size, either in number of element, and caches elements
 * using a specified eviction algorithm.
 *
 * This class IS thread safe
 */
template<class K, class V, class _Hash = std::hash<K>, typename StrongAssociation = std::false_type>
class SharedKvCache : public Module, boost::noncopyable {
    public:
     typedef K key_type;
     typedef V mapped_type;
     typedef _Hash hash_type;
     typedef std::size_t size_type;
     typedef boost::shared_ptr<mapped_type> value_type;
     typedef bool dirty_type;

    // Eviction list
    private:
     typedef std::tuple<key_type, value_type, dirty_type> entry_type;
     typedef std::list<entry_type> cache_type;

     typedef typename cache_type::iterator iterator;
     typedef typename cache_type::const_iterator const_iterator;

     // Backing data structure that implements key to value associated lookup.
     typedef typename std::unordered_map<key_type, iterator, hash_type> index_type;

    public:
     /**
      * Constructs the cache object but does not init
      * @param[in] modName     Name of this module
      * @param[in] _max_entries Maximum number of cache entries
      *
      * @return none
      */
     SharedKvCache(const std::string& module_name, size_type const _max_entries) :
         Module(module_name.c_str()),
         cache_map(),
         eviction_list(),
         current_size(0),
         max_entries(_max_entries),
         cache_lock() { }

     ~SharedKvCache() {}

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
      * @return A shared_ptr<T> to any evicted entry
      */
     value_type add(const key_type& key, const value_type value, const dirty_type dirty = false) {
         SCOPEDWRITE(cache_lock);

         if (StrongAssociation::value) {
             // Touch any exiting entry from cache, if returns
             // non-NULL then we already have this value, return
             if (touch(key) != eviction_list.end())
                 return value_type(nullptr);
         } else {
             // Remove old value before adding current
             if (!remove_(key, dirty)) {
                 return value_type(nullptr);
             }
         }

         // Add the entry to the front of the eviction list and into the map.
         eviction_list.emplace_front(key, value, dirty);
         cache_map[key] = eviction_list.begin();

         // Check if anything needs to be evicted
         if (++current_size > max_entries) {
             entry_type evicted = eviction_list.back();
             eviction_list.pop_back();

             value_type entryToEvict = std::get<1>(evicted);

             // Remove the cache iterator entry from the map
             cache_map.erase(std::get<0>(evicted));

             fds_verify(--current_size == max_entries);
             return entryToEvict;
         }

         // Return a NULL pointer if nothing was evicted
         return value_type(nullptr);
     }

     /**
      * Convenience add-by-value method. Copies incoming value
      * prior to inserting it into the cache line.
      *
      * @param[in] key   Key to use for indexing
      * @param[in] value Associated value
      *
      * @return A shared_ptr<T> to any evicted entry
      */
     value_type add(const key_type &key, mapped_type const& value, bool const write_update = false) {
         return add(key, boost::make_shared<mapped_type>(value), write_update);
     }

     /**
      * Removes all keys and values from the cache line
      *
      * @return none
      */
     void clear() {
         SCOPEDWRITE(cache_lock);
         // Clear all of the containers
         eviction_list.clear();
         cache_map.clear();
         current_size = 0;
     }

     /**
      * Returns the value for the assoicated key. When a value
      * is returned, the cache RETAINS ownership of pointer and
      * it cannot be freed by the caller.
      * When a value is return, the entry's access is updated.
      * 
      * @param[in]  key      Key to use for indexing
      * @param[out] value_out Pointer to value
      * @para[in]   do_touch  Whether to track this access
      *
      * @return ERR_OK if a value is returned, ERR_NOT_FOUND otherwise.
      */
    Error get(const key_type &key,
                       value_type& value_out,
                       fds_bool_t const do_touch = true) {
         SCOPEDWRITE(cache_lock);
         typename cache_type::iterator elemIt;

         // Touch the entry in the list if we want
         // to track this access
         if (do_touch == true) {
             elemIt = touch(key);
         } else {
             auto mapIt = cache_map.find(key);
             if (mapIt == cache_map.end())
                 elemIt = eviction_list.end();
             else
                 elemIt = mapIt->second;
         }

         if (elemIt == eviction_list.end()) {
             return ERR_NOT_FOUND;
         }

         // Set the return value
         value_out = std::get<1>(*elemIt);
         return ERR_OK;
     }

     /**
      * Removes a key and value from the cache. Thread safe.
      *
      * @param[in] key   Key to use for indexing
      *
      * @return none
      */
     void remove(const key_type &key) {
         SCOPEDWRITE(cache_lock);
         remove_(key, false);
     }

     /**
      * Removes a key and value from the cache when predicate == TRUE Thread safe.
      *
      * @param[in] pred   Unary predicate to test each element against.
      *
      * @return none
      */
     template<typename UnaryPredicate>
     void remove_if(UnaryPredicate pred) {
         SCOPEDWRITE(cache_lock);
         // Remove all elements who match the predicate
         for (auto cur = cache_map.begin(); cache_map.end() != cur; ) {
             if (pred(cur->first)) {
                 auto cacheEntry = cur->second;
                 // Remove from the cache_map
                 cur = cache_map.erase(cur);
                 // Remove from the eviction_list
                 eviction_list.erase(cacheEntry);
                 --current_size;
             } else {
                 ++cur;
             }
         }
     }

     /**
      * Checks if a key exists in the cache
      * 
      * @param[in]  key  Key to use for indexing
      *
      * @return true if key exists, false otherwise
      */
     fds_bool_t exists(const K &key) const {
         SCOPEDREAD(cache_lock);
         return (cache_map.find(key) != cache_map.end());
     }

     /**
      * Returns the current number of entries in the cache
      *
      * @return number of entries
      */
     size_type getNumEntries() const {
         SCOPEDREAD(cache_lock);
         return current_size;
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

    private:
     // Maximum number of entries in the cache
     size_type max_entries;

     // Cache line itself
     cache_type eviction_list;
     size_t current_size;

     // K->V association lookup
     index_type cache_map;

     // Synchronization members
     mutable fds_rwlock cache_lock;

    /**
      * Internal function to remove a key and value
      * from the cache.
      *
      * @param[in] key   Key to use for indexing
      *
      * @return none
      */
     bool remove_(const key_type &key, dirty_type const dirty) {
         // Locate the key in the map
         auto mapIt = cache_map.find(key);
         if (mapIt != cache_map.end()) {
             iterator cacheEntry = mapIt->second;

             // Only remove if the new element is not dirty or the existing is
             if (dirty && !std::get<2>(*cacheEntry)) {
                 LOGDEBUG << "Skipping cache of dirty entry.";
                 return false;
             }

             // Remove from the cache_map
             cache_map.erase(mapIt);
             // Remove from the eviction_list
             eviction_list.erase(cacheEntry);
             --current_size;
         }
         return true;
     }

     /**
      * Touches a key for the purpose of representing an access.
      *
      * @param[in]  key  Key to use for indexing
      *
      * @return ERR_OK if a value is returned, ERR_NOT_FOUND otherwise.
      */
     typename cache_type::iterator touch(const key_type &key) {
         auto mapIt = cache_map.find(key);
         if (mapIt == cache_map.end()) return eviction_list.end();

         iterator existingEntry = mapIt->second;

         // Move the entry's position to the front
         // of the eviction list.
         eviction_list.splice(eviction_list.begin(),
                              eviction_list,
                              existingEntry);

         return existingEntry;
     }
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_CACHE_SHAREDKVCACHE_H_

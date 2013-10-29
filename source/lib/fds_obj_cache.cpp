#include <fds_obj_cache.h>

namespace fds{

  VolObjectCache::VolObjectCache(fds_volid_t vol_id, fds_uint64_t min_cache_size, fds_uint64_t max_cache_size)
    : min_cache_size(min_cache_size),
      max_cache_size(max_cache_size),
      object_map(),
      vol_id(vol_id)
  {
    total_mem_used = ATOMIC_VAR_INIT(0);
    vol_cache_lock = new fds_mutex("Volume object cache");
  }

  FdsObjectCache::FdsObjectCache(fds_uint64_t cache_size, // Max size of the cache, across all volumes
				 int slab_allocator_type, // Should pick one of our predefined slab allocators.
				 int eviction_policy, // LRU etc .. Default policy for now will be LRU
				 fds_log *parent_log)
  : vol_cache_map(),
      max_cache_size(cache_size),
      slab_allocator_type(slab_allocator_type),
      cache_eviction_policy(eviction_policy),
      oc_log(parent_log),
      volmap_rwlock()
  {
    total_mem_used = ATOMIC_VAR_INIT(0);
    FDS_PLOG(oc_log) << "Cache initialized with a maximum size of " << max_cache_size
		     << ", slab allocator - " << slab_allocator_type
		     << ", and eviction policy - " << cache_eviction_policy;
  }

  FdsObjectCache::~FdsObjectCache() {

  }

  int FdsObjectCache::vol_cache_create (fds_volid_t vol_id, 
			  fds_uint64_t min_cache_size, // Will never take away cache space beyond this size 
			  fds_uint64_t max_cache_size) // Will never let the cache grow beyond this size 
  {
    VolObjectCache *vol_cache = new VolObjectCache(vol_id, min_cache_size, max_cache_size);
    volmap_rwlock.write_lock();
    vol_cache_map[vol_id] = vol_cache;
    volmap_rwlock.write_unlock();
  }

  int FdsObjectCache::vol_cache_delete(fds_volid_t vol_id) {
    volmap_rwlock.write_lock();
    VolObjectCache *vol_cache = NULL;
    if (vol_cache_map.count(vol_id) == 0) {
      volmap_rwlock.write_unlock();
      return -1;
    }
    vol_cache = vol_cache_map[vol_id];
    vol_cache_map.erase(vol_id);
    volmap_rwlock.write_unlock();
    delete vol_cache;
  }

  // Lookup an object
  ObjBufPtrType FdsObjectCache::object_get(fds_volid_t vol_id, ObjectID objId) {
    volmap_rwlock.read_lock();
    VolObjectCache *vol_cache = NULL;
    if (vol_cache_map.count(vol_id) == 0) {
      volmap_rwlock.write_unlock();
      return NULL;
    }
    vol_cache = vol_cache_map[vol_id];
    volmap_rwlock.read_unlock();
    vol_cache->vol_cache_lock->lock();
    ObjBufPtrType objBuf = NULL;
    if (vol_cache->object_map.count(objId) == 0) {
      vol_cache->vol_cache_lock->unlock();
      return NULL;
    }
    objBuf = vol_cache->object_map[objId];
    vol_cache->vol_cache_lock->unlock();
    return objBuf;
  }

  // Release ref cnt..do we need this if we use smart ptr?
  int FdsObjectCache::object_put(fds_volid_t vol_id, ObjectID objId, ObjBufPtrType obj_data) {
    // no op for now since we use smart pointers which gives us ref_cnt
  }

  // Simply allocate buffer, object will not be in any cache lookup until object add is done.
  // If the cache can grow, new buf space will be allocated.
  // Otherwise, a buffer from an existing object in the cache will be reallocated, using the eviction policy.
  ObjBufPtrType FdsObjectCache::object_alloc(fds_volid_t vol_id, 
					     ObjectID objId, 
					     fds_uint64_t obj_size) {

  } 
    
  // Add it to the cache map so it is available for future lookups.
  int FdsObjectCache::object_add(fds_volid_t vol_id, ObjectID objId, ObjBufPtrType *obj_data) {

  }

  // Delete this object from the cache map and use the buffer for reallocation for future alloc requests
  // Primarily to be used to by garbage collection thread
  ObjBufPtrType FdsObjectCache::object_delete(fds_volid_t vol_id, ObjectID objId) {

  }

}

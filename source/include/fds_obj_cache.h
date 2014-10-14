#ifndef __FDS_OBJ_CACHE_H_

#define __FDS_OBJ_CACHE_H_

#include <unordered_map>
#include <boost/shared_ptr.hpp>
#include <atomic>

#include <fds_types.h>
#include <fds_error.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>
#include <concurrency/RwLock.h>

using namespace std;

namespace fds {


  class ObjectCacheBuf : public ObjectBuf {
  public:
    fds_volid_t vol_id;
    ObjectID obj_id;
    bool io_in_progress;
    bool copy_is_dirty;
    void *plcy_data;
    // Some additional fields here to track it's eviction candidacy status
    // If we use calendar queues for example, 
    // we may store here the ptr to the calendar
    // item to which this object belongs.
    ObjectCacheBuf()
      : io_in_progress(false), copy_is_dirty(false)
      {
          data = boost::make_shared<std::string>(std::string(""));
          vol_id = 0;
          plcy_data = NULL;
      }
  };

  typedef boost::shared_ptr<ObjectBuf> ObjBufPtrType; // Smart pointer type using boost shared_ptr template
  typedef boost::shared_ptr<ObjectCacheBuf> ObjCacheBufPtrType; // Smart pointer type using boost shared_ptr template

  typedef std::unordered_map<ObjectID, ObjCacheBufPtrType, ObjectHash> vol_obj_map_t; // Per-volume map mapping an obj Id to obj buf

  enum slab_allocator_type {
    slab_allocator_type_fds
  };

  const int slab_allocator_type_default = slab_allocator_type_fds;

  class slab_object_allocator_base {

  public:
    virtual ObjCacheBufPtrType allocate_object_buf(fds_uint32_t obj_size) = 0;
    virtual void return_object_buf_to_pool(ObjCacheBufPtrType obj_buf) = 0;
  };

  enum eviction_policy_type {
    eviction_policy_type_lru
  };

  const int eviction_policy_type_default = eviction_policy_type_lru;

  class eviction_policy_manager_base {
  public:
    virtual void handle_object_access(fds_volid_t vol_id, ObjectID oid, ObjCacheBufPtrType objBuf) = 0;
    virtual void handle_object_delete(fds_volid_t vol_id, ObjectID oid, ObjCacheBufPtrType objBuf) = 0;
    virtual void evictObjectsFromVolumeCache(fds_volid_t vol_id, fds_uint64_t bytes_required) = 0;
    virtual void evictObjectsFromAnyCache(fds_uint64_t bytes_required) = 0;
  };

  class VolObjectCache {

  public:
    fds_volid_t vol_id;
    vol_obj_map_t object_map;
    std::atomic<fds_uint64_t> total_mem_used;
    fds_uint64_t num_cache_hits;
    fds_uint64_t num_cache_queries;
    fds_uint64_t num_objects_added;
    fds_uint64_t num_objects_evicted;

    fds_uint64_t min_cache_size;
    fds_uint64_t max_cache_size;

    fds_mutex *vol_cache_lock;

    VolObjectCache(fds_volid_t vol_id, fds_uint64_t min_cache_size, fds_uint64_t max_cache_size);
    ~VolObjectCache();

  };

  class FdsObjectCache {
    
  public:

    FdsObjectCache(fds_uint64_t cache_size, // Max size of the cache, across all volumes
		   int slab_allocator_type, // Should pick one of our predefined slab allocators.
		   int eviction_policy, // LRU etc .. Default policy for now will be LRU
		   fds_log *parent_log);
    ~FdsObjectCache();

    int vol_cache_create (fds_volid_t vol_id, 
			  fds_uint64_t min_cache_size, // Will never take away cache space beyond this size 
			  fds_uint64_t max_cache_size);// Will never let the cache grow beyond this size

    int vol_cache_delete(fds_volid_t vol_id);

    // Lookup an object
    ObjBufPtrType object_retrieve(fds_volid_t vol_id, ObjectID objId);

    // Release ref cnt..do we need this if we use smart ptr?
    int object_release(fds_volid_t vol_id, ObjectID objId, ObjBufPtrType& obj_data);

    // Simply allocate buffer, object will not be in any cache lookup until object add is done.
    // If the cache can grow, new buf space will be allocated.
    // Otherwise, a buffer from an existing object in the cache will be reallocated, using the eviction policy.
    ObjBufPtrType object_alloc(fds_volid_t vol_id, 
			       ObjectID objId, 
			       fds_uint64_t obj_size); 
    
    // Add it to the cache map so it is available for future lookups.
    int object_add(fds_volid_t vol_id, ObjectID objId, ObjBufPtrType& obj_data, bool is_dirty);

    // Was the object previously added as dirty? Call this after 
    // the object is synced to persistent storage.
    int mark_object_clean(fds_volid_t vol_id,
			  ObjectID objId,
			  ObjBufPtrType& obj_data);

    // Delete this object from the cache map and use the buffer for reallocation for future alloc requests
    // Primarily to be used to by an external garbage collection thread
    int object_delete(fds_volid_t vol_id, ObjectID objId);
    
    // A version of delete for the plcy manager to call when it wants to evict an object.
    // Main difference between this and delete is this assumes plcy manager already knows
    // about this object going away.
    int object_evict(fds_volid_t vol_id, ObjectID objId);

    // Check if objects can be evicted from a volume cache
    // without violating min_cache_sz requirement
    bool volume_evictable(fds_volid_t vol_id);

    bool is_object_buf_dirty(fds_volid_t vol_id,
			     ObjectID objId,
			     ObjBufPtrType& obj_data);

    bool is_object_io_in_progress(fds_volid_t vol_id,
				  ObjectID objId,
				  ObjBufPtrType& obj_data);

    void log_stats_to_file(std::string file_name);
    fds_log *GetLog() { return oc_log; }

   private:

    int slab_allocator_type;
    int cache_eviction_policy;
    fds_uint64_t max_cache_size;
    fds_log *oc_log;

    std::unordered_map <fds_volid_t, VolObjectCache *> vol_cache_map;
    slab_object_allocator_base *slab_allocator;
    eviction_policy_manager_base *plcy_mgr;
    std::atomic<fds_uint64_t> total_mem_used;
    fds_rwlock volmap_rwlock; // protects the volume map
                              // against volume create/delete events.
    
    void *lru_data; // TBD, a calendar queue probably. Priority queues are very space-intensive for large number of objects.
    ObjCacheBufPtrType object_remove(fds_volid_t vol_id, ObjectID objId, bool ignore_in_progress);

  };

}

#endif

/***********************************************************************************************************
 Sample code illustrating example usage
************************************************************************************************************/
/***********************************************************************************************************

daemon_init() {
//...
obj_cache = new FDSObjectCache(1*10^30, slab_type_default, eviction_plcy_default); 
//...
}

volume_create(vol_id) {

 obj_cache->vol_cache_create(vol_id, vol_cache_sz_min, vol_cache_sz_max);

}


object_read(vol_id, oid) {

 // 1. Check cache
 objBufPtr = object_cache->object_get(vol_id, obj_id);
 if (objBufPtr) {
   copy_data_and_respond_to_RPC(objBufPtr);
   object_cache->object_release(vol_id, obj_id, objBufPtr);
 }
 // 2. Cache miss. Lookup obj idx, get the object size and location, get a new object ptr allocated for the sz.
 indexdb_get_obj_locn(obj_id, &obj_size, &obj_locn); // from levelDB index DB
 objBufPtr = obj_cache->object_alloc(vol_id, obj_id, obj_size);

 // ... retrieve object from disk using persistence layer API
 persistent_layer_object_get(objBufPtr->data.c_str(), obj_locn, obj_size);

 // If persistence layer is invoked asynchronously, 
 // then the folllwing steps need to be happen in the completion callback. 
 copy_data_and_respond_to_RPC(objBufPtr);
 object_cache->object_release(vol_id, obj_id, objBufPtr);  
}

object_write(vol_id, oid, msg) {

 // 1. Check indexDB for dedup.
 rc = indexdb_get_object_locn_and_size(obj_id, &old_obj_locn, &old_obj_size);
 if (rc == ERR_OK) {
   // ... get the object from cache if present and check for dedup.
   // otherwise, retrieve from persistence layer and check.
 } 
 objBufPtr = obj_cache->object_alloc(vol_id, obj_id, msg->obj_size); 
 memcpy(objBufPtr->data.c_str(), msg->obj_data, msg->obj_size); 
 // Note, if cache is a non-volatile cache, we can add to cache and 
 // respond to write req right here and 
 // proceed with persisting in the background.

 persistent_layer_object_put(obj_id, objBufPtr->data.c_str(), obj_tier, &obj_locn);
 indexdb_update_obj_locn(vol_id, msg->obj_size, obj_locn); 
 
 // If persistence layer is invoked asynchronously, 
 // then the folllwing steps need to be happen in the completion callback. 
 object_cache->object_add(vol_id, obj_id, objBufPtr);
 respond_to_RPC();
 object_cache->object_release(vol_id, obj_id, objBufPtr);  

}

************************************************************************************************************/

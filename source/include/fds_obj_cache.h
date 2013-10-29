#include <unordered_map>
#include <boost/shared_ptr.h>

#include <fds_types.h>
#include <concurrency/Mutex.h>
#include <concurrency/RwLock.h>

namespace fds{

  class ObjectCacheBuf : public ObjectBuf {
  public:
    boost::posix_time::ptime last_access_time;
    bool io_in_progress;
    // Some additional fields here to track it's eviction candidacy status
    // If we use calendar queues for example, 
    // we may store here the ptr to the calendar
    // item to which this object belongs.
  }

  typedef boost::shared_ptr<ObjectBuf> ObjBufPtrType; // Smart pointer type using boost shared_ptr template
  
  typedef unordered_map<ObjectId, ObjBufPtrType> vol_obj_map_t; // Per-volume map mapping an obj Id to obj buf

  typedef enum {
    slab_allocator_type_fds;
  } slab_allocator_type_t;

  const slab_allocator_type_t slab_allocator_type_default = slab_allocator_type_fds;

  typedef enum {
    eviction_policy_type_lru;
  } eviction_policy_type_t;
  
  const eviction_policy_type_t eviction_policy_type_default = eviction_policy_type_lru;

  class VolObjectCache {
  public:
    vol_obj_map_t object_map;
    std::atomic<fds_uint64_t> total_mem_used;
    fds_uint64_t min_cache_size;
    fds_uint64_t max_cache_size;

    fds_mutex *vol_cache_lock;
  }

  class slab_allocator_t slab_allocator;

  class FdsObjectCache {

  public:

    FdsObjectCache(fds_uint64_t cache_size, // Max size of the cache, across all volumes
		   int slab_allocator_type, // Should pick one of our predefined slab allocators.
		   int eviction_policy); // LRU etc .. Default policy for now will be LRU 
    ~FdsObjectCache();

    int vol_cache_create (fds_volid_t vol_id, 
			  fds_uint64_t min_cache_size, // Will never take away cache space beyond this size 
			  fds_uint64_t max_cache_size);// Will never let the cache grow beyond this size

    int vol_cache_delete(fds_volid_t vol_id);

    // Lookup an object
    ObjBufPtrType object_get(fds_volid_t vol_id, ObjectId objId);

    // Release ref cnt..do we need this if we use smart ptr?
    int object_put(fds_volid_t vol_id, ObjectId objId, ObjBufPtrType obj_data);

    // Simply allocate buffer, object will not be in any cache lookup until object add is done.
    // If the cache can grow, new buf space will be allocated.
    // Otherwise, a buffer from an existing object in the cache will be reallocated, using the eviction policy.
    ObjBufPtrType object_alloc(fds_vol_id vol_id, 
			       ObjectId objId, 
			       fds_uint64_t obj_size); 
    
    // Add it to the cache map so it is available for future lookups.
    int object_add(fds_vol_id vol_id, ObjectId objId, ObjBufPtrType *obj_data);

    // Delete this object from the cache map and use the buffer for reallocation for future alloc requests
    // Primarily to be used to by garbage collection thread
    ObjBufPtrtype object_delete(fds_volid_t vol_id, ObjectId objId);

   private:

    int slab_allocator_type;
    int cache_eviction_policy;
    

    unordered_map <fds_volid_t, VolObjectCache *> vol_cache;
    slab_allocator_t *slab_allocator;
    std::atomic<fds_uint64_t> total_mem_used;
    fds_rwlock *volmap_rwlock; // protects the volume map
                              // against volume create/delete events.
    
    void *lru_data; // TBD, a calendar queue probably. Priority queues are very space-intensive for large number of objects.

  }

}

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
   object_cache->object_put(vol_id, obj_id, objBufPtr);
 }
 // 2. Cache miss. Lookup obj idx, get the object size and location, get a new object ptr allocated for the sz.
 indexdb_get_obj_locn(obj_id, &obj_size, &obj_locn); // from levelDB index DB
 objBufPtr = obj_cache->object_alloc(vol_id, obj_id, obj_size);

 // ... retrieve object from disk using persistence layer API
 persistent_layer_object_get(objBufPtr->data.c_str(), obj_locn, obj_size);

 // If persistence layer is invoked asynchronously, 
 // then the folllwing steps need to be happen in the completion callback. 
 copy_data_and_respond_to_RPC(objBufPtr);
 object_cache->object_put(vol_id, obj_id, objBufPtr);  
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
 object_cache->object_put(vol_id, obj_id, objBufPtr);  

}

************************************************************************************************************/

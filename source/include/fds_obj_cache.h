#include <unordered_map>
#include <memory>

namespace fds{

  typedef shared_ptr<ObjectData> ObjDataPtrType; // Smart pointer type using STL template
  
  typedef unordered_map<ObjectId, ObjDataPtrType> vol_obj_map_t;

  class VolObjectCache {
  public:
    vol_obj_map_t object_map;
    fds_uint64_t total_mem_used;
    fds_mutex_t vol_cache_lock;
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


    ObjDataPtrType object_get(fds_volid_t vol_id, ObjectId objId);
    int object_put(fds_volid_t vol_id, ObjectId objId, ObjDataPtrType obj_data);
    ObjDataPtrType object_alloc(fds_vol_id vol_id, ObjectId objId, fds_uint64_t obj_size);
    ObjDataPtrtype object_delete(fds_volid_t vol_id, ObjectId objId);

   private:

    int slab_allocator_type;
    int cache_eviction_policy;
    

    unordered_map <fds_volid_t, VolObjectCache *> vol_cache;
    slab_allocator_t *slab_allocator;
    fds_uint64_t total_mem_used;
    fds_rw_lock_t volmap_rwlock; // protects the volume map
                                 // against volume create/delete events.
    
    void *lru_data; // TBD, a priority heap or a calendar queue

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
 objDataPtr = object_cache->object_get(vol_id, obj_id);
 if (objDataPtr) {
   copy_data_and_respond_to_RPC(objDataPtr);
   object_cache->object_put(vol_id, obj_id);
 }
 // 2. Cache miss. Lookup obj idx, get the object size and location, get a new object ptr allocated for the sz.
 indexdb_get_obj_locn(obj_id, &obj_size, &obj_locn); // from levelDB index DB
 objDataPtr = obj_cache->object_alloc(vol_id, obj_id, obj_size);

 // ... retrieve object from disk using persistence layer API
 persistent_layer_object_get();

 // If persistence layer is invoked asynchronously, 
 // then the folllwing steps need to be happen in the completion callback. 
 copy_data_and_respond_to_RPC(objDataPtr);
 object_cache->object_put(vol_id, obj_id);  
}

object_write(vol_id, oid, msg) {

 // 1. Check indexDB for dedup.
 rc = indexdb_get_object_locn_and_size(obj_id, &old_obj_locn, &old_obj_size);
 if (rc == ERR_OK) {
   // ... get the object from cache if present and check for dedup.
   // otherwise, retrieve from persistence layer and check.
 } 
 objDataPtr = obj_cache->object_alloc(vol_id, obj_id, msg->obj_size); 
 memcpy(objDataPtr->data, msg->obj_data, msg->obj_size); 
 // Note, if cache is a non-volatile cache, we can respond to write 
 // req here and proceed with persisting in the background.

 persistent_layer_object_put(obj_id, objData, obj_tier);
 
 // If persistence layer is invoked asynchronously, 
 // then the folllwing steps need to be happen in the completion callback. 
 respond_to_RPC(objDataPtr);
 object_cache->object_put(vol_id, obj_id);  


}

************************************************************************************************************/

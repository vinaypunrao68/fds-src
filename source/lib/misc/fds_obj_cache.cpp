#include <fds_obj_cache.h>
#include <simple_object_allocator.h>
#include <lru_policy_mgr.h>
#include <fstream>

namespace fds{

  VolObjectCache::VolObjectCache(fds_volid_t vol_id, fds_uint64_t min_cache_size, fds_uint64_t max_cache_size)
    : min_cache_size(min_cache_size),
      max_cache_size(max_cache_size),
      object_map(),
      vol_id(vol_id)
  {
    total_mem_used = ATOMIC_VAR_INIT(0);
    num_cache_hits = 0;
    num_cache_queries = 0;
    num_objects_added = 0;
    num_objects_evicted = 0;
    vol_cache_lock = new fds_mutex("Volume object cache");
  }

  VolObjectCache::~VolObjectCache() {

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
    slab_allocator = new simple_object_allocator();
    plcy_mgr = new lru_policy_mgr(this);
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
    FDS_PLOG(oc_log) << " Volume cache created for volume " << vol_id
		     << " with min - " << min_cache_size 
		     << " and max - " << max_cache_size;
    return 0;
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
    return 0;
  }

  // Lookup an object
  ObjBufPtrType FdsObjectCache::object_retrieve(fds_volid_t vol_id, ObjectID objId) {
    volmap_rwlock.read_lock();
    VolObjectCache *vol_cache = NULL;
    if (vol_cache_map.count(vol_id) == 0) {
      volmap_rwlock.read_unlock();
      return NULL;
    }
    vol_cache = vol_cache_map[vol_id];
    volmap_rwlock.read_unlock();
    vol_cache->vol_cache_lock->lock();
    vol_cache->num_cache_queries++;
    ObjCacheBufPtrType objCacheBuf = NULL;
    if (vol_cache->object_map.count(objId) == 0) {
      vol_cache->vol_cache_lock->unlock();
      return NULL;
    }
    vol_cache->num_cache_hits++;
    objCacheBuf = vol_cache->object_map[objId];
    vol_cache->vol_cache_lock->unlock();
    //if (objCacheBuf->io_in_progress) {
    //  return NULL;
    //}
    plcy_mgr->handle_object_access(vol_id, objId, objCacheBuf);
    ObjBufPtrType objBuf =  boost::static_pointer_cast<ObjectBuf>(objCacheBuf);
    return objBuf;
  }

  // Release ref cnt..do we need this if we use smart ptr?
  int FdsObjectCache::object_release(fds_volid_t vol_id, ObjectID objId, ObjBufPtrType& obj_data) {
    // assign NULL to the passed in ptr reference, there by forcefully releasing the reference
    // held by the caller
    obj_data = NULL;
    return 0;
  }

  // Simply allocate buffer, object will not be in any cache lookup until object add is done.
  // If the cache can grow, new buf space will be allocated.
  // Otherwise, a buffer from an existing object in the cache will be reallocated, using the eviction policy.
  ObjBufPtrType FdsObjectCache::object_alloc(fds_volid_t vol_id, 
					     ObjectID objId, 
					     fds_uint64_t obj_size) {

    ObjBufPtrType objBufPtr = NULL;

    volmap_rwlock.read_lock();
    VolObjectCache *vol_cache = NULL;
    if (vol_cache_map.count(vol_id) == 0) {
      volmap_rwlock.read_unlock();
      return NULL;
    }
    vol_cache = vol_cache_map[vol_id];
    volmap_rwlock.read_unlock();

    fds_uint64_t current_vol_cache_sz = atomic_load(&vol_cache->total_mem_used);
    fds_uint64_t current_cache_sz = atomic_load(&total_mem_used);
    if (current_vol_cache_sz + obj_size >= vol_cache->max_cache_size) {
      // pick a candidate for eviction from the same volume.
      FDS_PLOG(oc_log) << "Evicting objects from volume " << vol_id
		       << " for " << obj_size << " bytes.";
      plcy_mgr->evictObjectsFromVolumeCache(vol_id, obj_size);
    } else if (current_cache_sz + obj_size >= max_cache_size) {
      // pick a candidate for eviction from any volume.
      FDS_PLOG(oc_log) << "Evicting objects from cache " << vol_id
		       << " for " << obj_size << " bytes.";
      plcy_mgr->evictObjectsFromAnyCache(obj_size);
    }

    //current_vol_cache_sz = atomic_load(&vol_cache->total_mem_used);
    //current_cache_sz = atomic_load(&total_mem_used);
    //assert(current_cache_sz + obj_size < max_cache_size);
    //assert(current_vol_cache_sz + obj_size < vol_cache->max_cache_size);
    
    ObjCacheBufPtrType newObjCacheBufPtr = slab_allocator->allocate_object_buf(obj_size);
    
    std::atomic_fetch_add(&vol_cache->total_mem_used, (fds_uint64_t) obj_size);
    std::atomic_fetch_add(&total_mem_used, (fds_uint64_t) obj_size);
    newObjCacheBufPtr->io_in_progress = true;
    newObjCacheBufPtr->copy_is_dirty = false;
    newObjCacheBufPtr->vol_id = vol_id;
    newObjCacheBufPtr->obj_id = objId;
    vol_cache->vol_cache_lock->lock();
    vol_cache->object_map[objId] = newObjCacheBufPtr;
    vol_cache->num_objects_added++;
    vol_cache->vol_cache_lock->unlock();

    FDS_PLOG(oc_log) << "Allocating " << obj_size << " bytes for object " 
		    << objId << " in volume " << vol_id << " in cache";

    objBufPtr = boost::static_pointer_cast<ObjectBuf>(newObjCacheBufPtr);    
    return objBufPtr;

  } 
    
  // Add it to the cache map so it is available for future lookups.
  int FdsObjectCache::object_add(fds_volid_t vol_id, 
				 ObjectID objId, 
				 ObjBufPtrType& obj_data,
				 bool is_dirty) {
    ObjCacheBufPtrType obj_cache_buf = boost::static_pointer_cast<ObjectCacheBuf>(obj_data);
    plcy_mgr->handle_object_access(vol_id, objId, obj_cache_buf);
    obj_cache_buf->io_in_progress = false;
    obj_cache_buf->copy_is_dirty = is_dirty;
    FDS_PLOG(oc_log) << "Added object " << objId << " in volume " << vol_id << " onto cache"
                     << " with dirty flag " << (is_dirty?"set":"cleared");
    return 0;
  }

  int FdsObjectCache::mark_object_clean(fds_volid_t vol_id,
					ObjectID objId,
					ObjBufPtrType& obj_data) {
    ObjCacheBufPtrType obj_cache_buf = boost::static_pointer_cast<ObjectCacheBuf>(obj_data);
    obj_cache_buf->copy_is_dirty = false;
    FDS_PLOG(oc_log) << "Marked object " << objId << " in volume " << vol_id << " clean";
    return 0;
  }


  bool FdsObjectCache::is_object_buf_dirty(fds_volid_t vol_id,
					   ObjectID objId,
					   ObjBufPtrType& obj_data) {
    ObjCacheBufPtrType obj_cache_buf = boost::static_pointer_cast<ObjectCacheBuf>(obj_data);
    return (obj_cache_buf->copy_is_dirty);
  }

  bool FdsObjectCache::is_object_io_in_progress(fds_volid_t vol_id,
						ObjectID objId,
						ObjBufPtrType& obj_data) {
    ObjCacheBufPtrType obj_cache_buf = boost::static_pointer_cast<ObjectCacheBuf>(obj_data);
    return (obj_cache_buf->io_in_progress);
  }

  

  // Internal helper function doign the job of removing the object from the volume index,
  // after doing all sanity checks.
  ObjCacheBufPtrType FdsObjectCache::object_remove(fds_volid_t vol_id, ObjectID objId, bool ignore_in_progress) {

    volmap_rwlock.read_lock();
    VolObjectCache *vol_cache = NULL;
    if (vol_cache_map.count(vol_id) == 0) {
      volmap_rwlock.write_unlock();
      FDS_PLOG(oc_log) << "Volume " << vol_id << " not found. Object removal failed for " << objId;
      return NULL;
    }
    vol_cache = vol_cache_map[vol_id];
    volmap_rwlock.read_unlock();
    vol_cache->vol_cache_lock->lock();

    if (vol_cache->object_map.count(objId) == 0) {
      vol_cache->vol_cache_lock->unlock();
      FDS_PLOG(oc_log) << "Volume " << vol_id << " does not have object " << objId
		      << ". Object removal failed";
      return NULL;
    }

    ObjCacheBufPtrType& objBuf = vol_cache->object_map[objId];

    assert((objBuf->copy_is_dirty == false) && 
	   ((ignore_in_progress) || (objBuf->io_in_progress == false)));

    if (objBuf.use_count() > 3) { 
      // Volume index tbl and the eviction_plcy_mgr are the only ones that should have a reference.
        // TODO(Andrew): Actually we just created a refernce locally too, so the count
        // is expected to be three
      vol_cache->vol_cache_lock->unlock();
      FDS_PLOG(oc_log) << "Volume " << vol_id << ", object " << objId
		       << "ref cnt - " << objBuf.use_count()
		       << ". Object removal failed";
      return NULL;
    }

    ObjCacheBufPtrType delObjBuf = vol_cache->object_map[objId];
    vol_cache->object_map.erase(objId);
    vol_cache->num_objects_evicted++;
    vol_cache->vol_cache_lock->unlock();
    std::atomic_fetch_sub(&vol_cache->total_mem_used, (fds_uint64_t) delObjBuf->getSize());
    std::atomic_fetch_sub(&total_mem_used, (fds_uint64_t) delObjBuf->getSize());
    return delObjBuf; // upto the caller to delete the object

  }

  // Notify plcy manager about removal, remove the object from the indices,
  // and return buff to object pool.
  // Primarily for an external garbage collector to call.
  int FdsObjectCache::object_delete(fds_volid_t vol_id, ObjectID objId) {
      // int rc;
    
    ObjCacheBufPtrType objBuf = object_remove(vol_id, objId, /* ignore_in_progress= */true);
    if (objBuf == NULL) {
      return -1;
    }

    // Update eviction policy manager here about the object going away
    plcy_mgr->handle_object_delete(vol_id, objId, objBuf);

    slab_allocator->return_object_buf_to_pool(objBuf);
    return 0;
  }

  // This is similar to object_delete but does not call plcy_manager to notify about object deletion.
  // This is for plcy_manager to call when it decides to evict an object.
  int FdsObjectCache::object_evict(fds_volid_t vol_id, ObjectID objId) {
    ObjCacheBufPtrType objBuf = object_remove(vol_id, objId, /*ignore_in_progress=*/false);
    if (objBuf == NULL) {
      return -1;
    }
    slab_allocator->return_object_buf_to_pool(objBuf);
    FDS_PLOG(oc_log) << "Evicted object " << objId << " in volume " << vol_id << " from cache";
    return 0;
  }

  bool FdsObjectCache::volume_evictable(fds_volid_t vol_id) {

    volmap_rwlock.read_lock();
    VolObjectCache *vol_cache = NULL;
    if (vol_cache_map.count(vol_id) == 0) {
      volmap_rwlock.write_unlock();
      return false;
    }
    vol_cache = vol_cache_map[vol_id];
    volmap_rwlock.read_unlock();
    
    fds_uint64_t current_vol_cache_sz = atomic_load(&vol_cache->total_mem_used);
    
    if (current_vol_cache_sz <= vol_cache->min_cache_size) {
      return false;
    }

    return true;
  }

  void FdsObjectCache::log_stats_to_file(std::string file_name) {

    volmap_rwlock.read_lock();
    for (auto it=vol_cache_map.begin(); it != vol_cache_map.end(); ++it)  {

      fds_volid_t vol_id = it->first;
      VolObjectCache *vol_cache = it->second;
      
      vol_cache->vol_cache_lock->lock();

      fds_uint64_t current_cache_size = atomic_load(&vol_cache->total_mem_used);
      fds_uint64_t hits = vol_cache->num_cache_hits;
      fds_uint64_t queries = vol_cache->num_cache_queries;
      fds_uint64_t num_objects_added = vol_cache->num_objects_added;
      fds_uint64_t num_objects_evicted = vol_cache->num_objects_evicted;

      vol_cache->vol_cache_lock->unlock();

      ofstream ofile;

      ofile.open(file_name, ios::out|ios::app);
      
      ofile << "Volid - " << vol_id 
	    << " ; Size - " << current_cache_size
	    << " ; Hits - " << hits << "/" << queries
	    << " ; Loads - " << num_objects_added
	    << " ; Evicted - " << num_objects_evicted
	    << endl;

      ofile.close();

    }
    volmap_rwlock.read_unlock();
    

  }

}


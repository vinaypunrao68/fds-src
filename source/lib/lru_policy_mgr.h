#ifndef __LRU_PLCY_MGR_H_

#define LRU_PLCY_MGR_H

#include <list>
#include <fds_obj_cache.h> 

namespace fds {

  typedef std::list<ObjCacheBufPtrType> EvictionList;

  typedef EvictionList::iterator EvictionListItrtr;

  class LRUData {
  public:
    EvictionListItrtr itr; // list iterator to access its position in the above list
    LRUData()
      {
      }
  };

  class lru_policy_mgr: public eviction_policy_manager_base {

  private:
    FdsObjectCache *parent_cache;
    EvictionList *lru_queue;
    fds_mutex *lru_lock;

  public:
    
    lru_policy_mgr(FdsObjectCache *prnt_cache)
      : parent_cache(prnt_cache)
      {
	lru_queue = new EvictionList();
	lru_lock = new fds_mutex("LRU Plcy Mgr Lock");
      } 

    void handle_object_access(fds_volid_t vol_id, ObjectID oid, ObjCacheBufPtrType objBuf) {
      
      LRUData *object_lru_data = NULL;
      EvictionListItrtr itr;

      lru_lock->lock();

      if (objBuf->plcy_data) {
	object_lru_data = static_cast<LRUData *>(objBuf->plcy_data);
	itr = object_lru_data->itr;
	lru_queue->erase(itr);
      } else {
	object_lru_data = new LRUData();
      }

      itr = lru_queue->insert(lru_queue->end(), objBuf);
      object_lru_data->itr = itr;

      lru_lock->unlock();

    }

    void handle_object_delete(fds_volid_t vol_id, ObjectID oid, ObjCacheBufPtrType objBuf) {
      if (objBuf->plcy_data) {
	LRUData *object_lru_data = static_cast<LRUData *>(objBuf->plcy_data);
	EvictionListItrtr itr = object_lru_data->itr;
	lru_lock->lock();
	lru_queue->erase(itr);
	lru_lock->unlock();
      }
    }

    void evictObjectsFromVolumeCache(fds_volid_t vol_id, fds_uint64_t bytes_required) {

      fds_uint64_t bytes_reclaimed = 0;
      
      lru_lock->lock();

      for (auto it = lru_queue->begin(); it != lru_queue->end(); ++it) {
	ObjCacheBufPtrType objBuf = *it;
	if (objBuf->vol_id == vol_id) {	  
	  bytes_reclaimed += objBuf->size;
	  parent_cache->object_evict(objBuf->vol_id, objBuf->obj_id);
	}
	if (bytes_reclaimed >= bytes_required) {
	  break;
	}
      }

      lru_lock->unlock();

    }

    void evictObjectsFromAnyCache(fds_uint64_t bytes_required) {

      fds_uint64_t bytes_reclaimed = 0;

      lru_lock->lock();

      for (auto it = lru_queue->begin(); it != lru_queue->end(); ++it) {
	ObjCacheBufPtrType objBuf = *it;
	if (parent_cache->volume_evictable(objBuf->vol_id)) {	  
	  bytes_reclaimed += objBuf->size;
	  parent_cache->object_evict(objBuf->vol_id, objBuf->obj_id);
	}
	if (bytes_reclaimed >= bytes_required) {
	  break;
	}
      }

      lru_lock->unlock();

    }
  };

}

#endif

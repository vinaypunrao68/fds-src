#ifndef __LRU_PLCY_MGR_H_

#define LRU_PLCY_MGR_H

#include <fds_obj_cache.h> 

namespace fds {

  class lru_policy_mgr: public eviction_policy_manager_base {

  private:
    FdsObjectCache *parent_cache;

  public:
    
    lru_policy_mgr(FdsObjectCache *prnt_cache)
      : parent_cache(prnt_cache)
      {

      } 
    void handle_object_access(fds_volid_t vol_id, ObjectID oid, ObjCacheBufPtrType objBuf) {
      
    }
    void handle_object_delete(fds_volid_t vol_id, ObjectID oid, ObjCacheBufPtrType objBuf) {

    }
    void evictObjectsFromVolumeCache(fds_volid_t vol_id, fds_uint64_t bytes_required) {

    }
    void evictObjectsFromAnyCache(fds_uint64_t bytes_required) {

    }
  };

}

#endif

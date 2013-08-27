/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Cache class for caching volume catalog entries on
 * the client side. The cache maintains a volatile db
 * and performs lookups to the data manager when needed.
 */

#ifndef SOURCE_STOR_HVISOR_ICE_VOLUMECATALOGCACHE_H_
#define SOURCE_STOR_HVISOR_ICE_VOLUMECATALOGCACHE_H_

#include <lib/Ice-3.5.0/cpp/include/Ice/Ice.h>
#include <lib/Ice-3.5.0/cpp/include/IceUtil/IceUtil.h>

#include <unordered_map>
#include <stdexcept>

#include "include/fds_err.h"
#include "include/fds_types.h"
#include "fdsp/FDSP.h"
#include "util/Log.h"
#include "util/concurrency/Mutex.h"
#include "util/concurrency/RwLock.h"

/*
 * Forward declaration of SH control class
 */
class StorHvCtrl;

namespace fds {

  /*
   * A catalog cache class for a single volume instance.
   */
  class CatalogCache {
 private:
    /*
     * Maps a volume offset to a object ID.
     */
    std::unordered_map<fds_uint32_t, ObjectID> offset_map;

    /*
     * Protects the offset_map.
     */
    fds_rwlock map_rwlock;

 public:
    CatalogCache();
    ~CatalogCache();

    /*
     * Update interface for a blob that's hosting a block device.
     */
    Error Update(fds_uint64_t block_id, const ObjectID& oid);
    Error Query(fds_uint64_t block_id, ObjectID *oid);
    void Clear();
  };

  /*
   * TODO: This should be broken up into the volume object
   * that will eventually be implemented on the client-side.
   * That means there won't need to be this map to each
   * volume cache, instead a map to the volumes.
   */
  class VolumeCatalogCache {
 private:
    /*
     * TODO: Use shared or unique pointers instead of
     * raw pointers. It's safer.
     */
    std::unordered_map<fds_uint32_t, CatalogCache*> vol_cache_map;

    /*
     * Protects the vol_cache_map.
     */
    fds_rwlock map_rwlock;

    /*
     * Reference to parent SH instance.
     */
    StorHvCtrl *parent_sh;

    /*
     * Pointer to logger to use.
     */
    fds_log    *vcc_log;
    fds_bool_t  created_log;

 public:

    /*
     * A logger is created for you if not passed in.
     */
    VolumeCatalogCache(StorHvCtrl *sh_ctrl);
    VolumeCatalogCache(StorHvCtrl *sh_ctrl,
                       fds_log *parent_log);
    
    VolumeCatalogCache();
    ~VolumeCatalogCache();

    Error RegVolume(fds_uint64_t vol_uuid);
    Error Query(fds_uint64_t vol_uuid,
                 fds_uint64_t block_id,
                 fds_uint32_t trans_id,
                 ObjectID *oid);
    Error Update(fds_uint64_t vol_uuid,
                 fds_uint64_t block_id,
                 const ObjectID &oid);

    void Clear(fds_uint64_t vol_uuid);

  };
}  // namespace fds

#endif  // SOURCE_STOR_HVISOR_ICE_VOLUMECATALOGCACHE_H_

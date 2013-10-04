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

#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>

#include <unordered_map>
#include <stdexcept>

#include <fds_err.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <fdsp/FDSP.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>
#include <concurrency/RwLock.h>

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
     * ID of vol associate with the cache.
     */
    fds_volid_t vol_id;

    /*
     * Local cache class for this volume's catalog.
     */
    CatalogCache cat_cache;

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
    VolumeCatalogCache(fds_volid_t _vol_id,
                       StorHvCtrl *sh_ctrl);
    VolumeCatalogCache(fds_volid_t _vol_id,
                       StorHvCtrl *sh_ctrl,
                       fds_log *parent_log);

    VolumeCatalogCache();
    ~VolumeCatalogCache();

    Error Query(fds_uint64_t block_id,
                fds_uint32_t trans_id,
                ObjectID *oid);
    Error Update(fds_uint64_t block_id,
                 const ObjectID &oid);
    void Clear();

    fds_volid_t GetID() const {
      return vol_id;
    }
  };
}  // namespace fds

#endif  // SOURCE_STOR_HVISOR_ICE_VOLUMECATALOGCACHE_H_

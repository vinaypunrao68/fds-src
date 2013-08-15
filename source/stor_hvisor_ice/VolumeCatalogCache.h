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

#include <unordered_map>
#include <stdexcept>

#include "include/fds_err.h"
#include "include/fds_types.h"

#include "lib/Ice-3.5.0/cpp/include/Ice/Ice.h"
#include "fdsp/FDSP.h"

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

 public:
    CatalogCache();
    ~CatalogCache();

    /*
     * Update interface for a blob that's hosting a block device.
     */
    Error Update(fds_uint64_t block_id, const ObjectID& oid);

    Error Query(fds_uint64_t block_id, ObjectID *oid);

  };

  /*
   * TODO: This should be broken up into the volume object
   * that will eventually be implemented on the client-side.
   * That means there won't need to be this map to each
   * volume cache, instead a map to the volumes.
   */
  class VolumeCatalogCache {
 private:
    std::unordered_map<fds_uint32_t, CatalogCache> vol_cache_map;

    /*
      TODO: Change variable name
     */
    FDS_ProtocolInterface::FDSP_DataPathReqPrx& fdspDPAPI;

 public:
    /*
     * TODO: Change this interface once we get a generic network
     * API. This current interface can ONLY talk to ONE DM!
     */
    VolumeCatalogCache(FDS_ProtocolInterface::FDSP_DataPathReqPrx& fdspDPAPI_arg);
    VolumeCatalogCache();
    ~VolumeCatalogCache();

    Error RegVolume(fds_uint64_t vol_uuid);
    Error Update(fds_uint64_t vol_uuid,
                 fds_uint64_t block_id,
                 const ObjectID &oid);
    
  }; 
}  // namespace fds

#endif  // SOURCE_STOR_HVISOR_ICE_VOLUMECATALOGCACHE_H_

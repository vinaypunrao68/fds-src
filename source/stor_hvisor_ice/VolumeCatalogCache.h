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
      TODO: Change variable name
     */
    FDS_ProtocolInterface::FDSP_DataPathReqPrx& fdspDPAPI;

 public:
    /*
     * TODO: Change this interface once we get a generic network
     * API. This current interface can ONLY talk to ONE DM!
     * The non-const reference here is OK.
     */
    VolumeCatalogCache(FDS_ProtocolInterface::FDSP_DataPathReqPrx&
                       fdspDPAPI_arg); // NOLINT(*)
    VolumeCatalogCache();
    ~VolumeCatalogCache();

    Error RegVolume(fds_uint64_t vol_uuid);
    Error Query(fds_uint64_t vol_uuid,
                 fds_uint64_t block_id,
                 ObjectID *oid);
    Error Update(fds_uint64_t vol_uuid,
                 fds_uint64_t block_id,
                 const ObjectID &oid);

    void Clear(fds_uint64_t vol_uuid);
  };
}  // namespace fds

#endif  // SOURCE_STOR_HVISOR_ICE_VOLUMECATALOGCACHE_H_

/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Cache class for caching volume catalog entries on
 * the client side. The cache maintains a volatile db
 * and performs lookups to the data manager when needed.
 */

#ifndef SOURCE_STOR_HVISOR_VOLUMECATALOGCACHE_H_
#define SOURCE_STOR_HVISOR_VOLUMECATALOGCACHE_H_

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <utility>

#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
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
    /// Describes an object's ID and length
    typedef std::pair<ObjectID, fds_uint32_t> ObjectDesc;
    /// Maps a blob offset to a object ID.
    typedef std::unordered_map<fds_uint64_t,
                               ObjectDesc> OffsetMap;
    OffsetMap offset_map;

    /*
     * Maintians metadata for the blob
     */
    std::string blobName;
    fds_uint64_t blobSize;
    std::string blobEtag;

    /*
     * Protects the offset_map.
     */
    fds_rwlock map_rwlock;

  public:
    explicit CatalogCache(const std::string &name);
    ~CatalogCache();

    /*
     * Update interface for a blob that's hosting a block device.
     */
    Error Update(fds_uint64_t blobOffset,
                 fds_uint32_t objectLen,
                 const ObjectID& oid,
                 fds_bool_t lastBuf);
    Error Query(fds_uint64_t blobOffset,
                ObjectID *oid);
    fds_uint64_t getBlobSize();
    std::string getBlobEtag();
    void setBlobEtag(const std::string &etag);
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

    /**
     * Local map to catalog/offset cache for each blob
     * TODO(Andrew): Make these smart pointers, not raw.
     */
    typedef std::unordered_map<std::string, CatalogCache *> BlobMap;
    BlobMap blobMap;
    fds_rwlock blobRwLock;

    /*
     * Reference to parent SH instance.
     */
    StorHvCtrl *parent_sh;

    /*
     * Pointer to logger to use.
     */
    fds_log    *vcc_log;
    fds_bool_t  created_log;

    /*
     * Function for issuing DM queries
     */
    Error queryDm(const std::string& blobName,
                  fds_uint64_t blobOffset,
                  fds_uint32_t trans_id,
                  ObjectID *oid);

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

    bool LookupObjectId(const std::string &blobName,
                         const uint64_t &offset, ObjectID &objId);
    Error Query(const std::string& blobName,
                fds_uint64_t blobOffset,
                fds_uint32_t trans_id,
                ObjectID *oid);
    Error Update(const std::string& blobName,
                 fds_uint64_t blobOffset,
                 fds_uint32_t objectLen,
                 const ObjectID &oid,
                 fds_bool_t lastBuf = false);
    Error getBlobSize(const std::string& blobName,
                      fds_uint64_t *blobSize);
    Error getBlobEtag(const std::string& blobName,
                      std::string *blobEtag);
    Error setBlobEtag(const std::string &blobName,
                      const std::string &blobEtag);
    Error clearEntry(const std::string &blobName);
    void Clear();

    fds_volid_t GetID() const {
      return vol_id;
    }
};
}  // namespace fds

#endif  // SOURCE_STOR_HVISOR_VOLUMECATALOGCACHE_H_

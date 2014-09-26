/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVCCACHE_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVCCACHE_H_

#include <string>
#include <vector>
#include <fds_module.h>
#include <fds_error.h>
#include <dm-vol-cat/DmExtentTypes.h>
#include <cache/VolumeKvCache.h>

namespace fds {

class DmCacheVolCatalog : public Module, boost::noncopyable {
  public:
    explicit DmCacheVolCatalog(const std::string &modName);
    ~DmCacheVolCatalog();

    /**
     * Module methods
     */
    int mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    /**
     * Creates volume cache for volume described in
     * volume descriptor 'vol_desc'.
     * The cache is initialized as part of creation.
     */
    Error createCache(const VolumeDesc& volDesc);

    /**
     * Removes volume cache for volume.
     * Any dirty entries must be flushed back to persistent
     * storage prior to removal, otherwise they will be lost.
     */
    Error removeCache(fds_volid_t volId);

    /**
     * Retrieves extent from the cache layer for given
     * volume id, blob_name. If extent is not found, returns error.
     * The cache returns a COPY of the cached entry that the caller
     * can use. The original copy remains owned by the cache. Once
     * the cache can properly pin/refcnt elements, we can consider
     * returning a direct pointer to the cache's memory.
     * If the extent ID queried is 0, an BlobExtent0 object will be
     * allocated. Otherwise a BlobExtent object is allocated. Since the
     * base BlobExtent is returned, it is up to the caller to cast to BlobExtent0.
     * @param[out] error will contain ERR_OK on success; ERR_CAT_ENTRY_NOT_FOUND
     * if extent0 does not exist in the cache layer; or other error.
     * @return BlobExtent0 containing extent from persistent layer; If
     * err is ERR_NOT_FOUND, BlobExtent0 is allocated but not filled in;
     * returns null pointer in other cases.
     */
    BlobExtent::ptr getExtent(fds_volid_t volume_id,
                               const std::string& blob_name,
                               fds_extent_id extent_id,
                               Error& error);

    /**
     * Updates an single extent in the cache for given volume id, blob name.
     * Any previously existing extent will be overwritten.
     */
    Error putExtent(fds_volid_t volume_id,
                    const std::string& blob_name,
                    const BlobExtent::const_ptr& extent);

    /**
     * Updates extent 0 and a set of non-0 extents in the cache
     * for given volume id, blob name.
     * This update is atomic -- either all updates are applied or none.
     * Any previously existing extents will be overwritten. If any extents
     * have been invalidated by an updated, those extents need to be removed
     * manually. The cache will not remove entries related to truncate or overwrite.
     */
    Error putExtents(fds_volid_t volume_id,
                     const std::string& blob_name,
                     const BlobExtent0::const_ptr& meta_extent,
                     const std::vector<BlobExtent::const_ptr>& extents);

    /**
     * Removes an extent from the cache.
     */
    Error removeExtent(fds_volid_t volume_id,
                       const std::string& blob_name,
                       fds_extent_id extent_id);

  private:
    // TODO(Andrew): Have a structure to track dirty cache entries
    // TODO(Andrew): Have a thread that periodically flushes dirty entries

    // TODO(Andrew): Have some per-volume configurable number of entries. Since
    // we don't change this yet, everyone gets the same.
    static const fds_uint32_t maxEntries = 200;
    // TODO(Andrew): Have some per-volume eviction policy based on the volume
    // policy.
    static const EvictionType evictionType = LRU;

    typedef VolumeCacheManager<ExtentKey, BlobExtent, ExtentKeyHash> VolumeBlobCacheManager;
    std::unique_ptr<VolumeBlobCacheManager> volCacheMgr;

    /// Temp rwlock until we move to a shared cache
    /// Protects the cache read and copy so that the ptr
    /// we get from the cache doesn't get freed in the meantime.
    fds_rwlock dmCacheRwLock;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVCCACHE_H_


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
     * Update or add extent 0 to the volume catalog cache
     * for given volume_id,blob_name
     */
    Error putMetaExtent(fds_volid_t volume_id,
                        const std::string& blob_name,
                        const BlobExtent0::const_ptr& meta_extent);

    /**
     * Updates extent 0 and a set of non-0 extents in the volume catalog
     * cache for given volume id, blob name.
     * This update is atomic -- either all updates are applied or none.
     * If extent 0 or any other extens do not exist, they are added to the DB
     * If any non-zero extent contains no offset to object info mappings, this
     * extent is deleted from the DB.
     */
    Error putExtents(fds_volid_t volume_id,
                     const std::string& blob_name,
                     const BlobExtent0::const_ptr& meta_extent,
                     const std::vector<BlobExtent::const_ptr>& extents);

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
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVCCACHE_H_


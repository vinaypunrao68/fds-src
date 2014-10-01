/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_

#include <string>
#include <fds_module.h>
#include <fds_volume.h>
#include <blob/BlobTypes.h>
#include <cache/VolumeKvCache.h>
#include <am-tx-mgr.h>

namespace fds {

/**
 * A client-side cache of blob metadata and data. The cache
 * multiplexes different volumes and allows for different
 * policy management for each volume.
 */
class AmCache : public Module, public boost::noncopyable {
  public:
    explicit AmCache(const std::string &modName);
    ~AmCache();
    typedef std::unique_ptr<AmCache> unique_ptr;

    /**
     * Module methods
     */
    int mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    /**
     * Creates cache structures for the volume described
     * in the volume descriptor.
     */
    Error createCache(const VolumeDesc& volDesc);

    /**
     * Removes volume cache for the volume.
     * Any dirty entries must be flushed back to persistent
     * storage prior to removal, otherwise they will be lost.
     */
    Error removeCache(fds_volid_t volId);

    /**
     * Retrieves blob descriptor from cache for given volume
     * and blob. If descriptor is not found, returns error.
     * The cache returns a COPY of the cached entry that the caller
     * can use. The original copy remains owned by the cache. Once
     * the cache can properly pin/refcnt elements, we can consider
     * returning a direct pointer to the cache's memory.
     */
    BlobDescriptor::ptr getBlobDescriptor(fds_volid_t volId,
                                          const std::string &blobName,
                                          Error &error);

    /**
     * Retrieves object ID from cache for given volume, blob,
     * and offset. If blob offset is not found, returns error.
     */
    ObjectID::ptr getBlobOffsetObject(fds_volid_t volId,
                                      const std::string &blobName,
                                      fds_uint64_t blobOffset,
                                      Error &error);

    /**
     * Retrieves object data from cache for given volume and object.
     * If object is not found, returns error.
     */
    boost::shared_ptr<std::string> getBlobObject(fds_volid_t volId,
                                                 const ObjectID &objectId,
                                                 Error &error);

    /**
     * Updates the cache with the contents from a commited
     * transaction. Any previously existing info will be
     * overwritten.
     */
    Error putTxDescriptor(const AmTxDescriptor::ptr &txDesc);

    /**
     * Updates a blob descriptor in the cache for given volume id
     * and blob name. Any previously existing descriptor will be
     * overwritten.
     */
    Error putBlobDescriptor(fds_volid_t volId,
                            const std::string &blobName,
                            const BlobDescriptor::ptr &blobDesc);

    /**
     * Removes cache entries for a specific blob in a volume.
     */
    Error removeBlob(fds_volid_t volId,
                     const std::string &blobName);

  private:
    /// Cache of blob descriptors
    typedef VolumeCacheManager<std::string, BlobDescriptor> BlobDescCacheManager;
    std::unique_ptr<BlobDescCacheManager> blobDescCache;

    /// Cache of blob offset mappings
    typedef VolumeCacheManager<BlobOffsetPair, ObjectID,
                               BlobOffsetPairHash> BlobOffsetCacheManager;
    std::unique_ptr<BlobOffsetCacheManager> blobOffsetCache;

    /// Cache of blob objects
    typedef VolumeCacheManager<ObjectID, std::string,
                               ObjectHash> BlobObjectCacheManager;
    std::unique_ptr<BlobObjectCacheManager> blobObjectCache;

    /// Max number of entries per volume cache
    fds_uint32_t maxEntries;
    /// Cache eviction policy
    // TODO(Andrew): Have some per-volume eviction policy based on volume policy.
    EvictionType evictionType;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_

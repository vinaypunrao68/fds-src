/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_

#include <string>
#include <fds_volume.h>
#include <blob/BlobTypes.h>
#include <cache/VolumeSharedKvCache.h>

namespace fds {

struct AmTxDescriptor;

/**
 * A client-side cache of blob metadata and data. The cache
 * multiplexes different volumes and allows for different
 * policy management for each volume.
 */
class AmCache {
    typedef VolumeSharedCacheManager<std::string, BlobDescriptor>
        descriptor_cache_type;
    typedef VolumeSharedCacheManager<BlobOffsetPair, ObjectID, BlobOffsetPairHash>
        offset_cache_type;
    typedef VolumeSharedCacheManager<ObjectID, std::string, ObjectHash, std::true_type>
        object_cache_type;

  public:
    AmCache();
    AmCache(AmCache const&) = delete;
    AmCache& operator=(AmCache const&) = delete;
    ~AmCache();

    /**
     * Creates cache structures for the volume described
     * in the volume descriptor.
     */
    Error registerVolume(fds_volid_t const vol_uuid, size_t const num_objs, bool const can_cache_meta);

    /**
     * Removes metadata cache for the volume.
     */
    void invalidateMetaCache(fds_volid_t const volId);

    /**
     * Removes volume cache for the volume.
     * Any dirty entries must be flushed back to persistent
     * storage prior to removal, otherwise they will be lost.
     */
    Error removeVolume(fds_volid_t const volId);

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
    Error getBlobOffsetObjects(fds_volid_t volId,
                               const std::string &blobName,
                               fds_uint64_t const obj_offset,
                               fds_uint64_t const obj_offset_end,
                               size_t const obj_size,
                               std::vector<ObjectID::ptr>& obj_ids);

    /**
     * Retrieves object data from cache for given volume and object ids.
     * Returns hit_cnt, miss_cnt
     */
    std::pair<size_t, size_t> getObjects(fds_volid_t volId,
                                         std::vector<ObjectID::ptr> const& objectIds,
                                         std::vector<boost::shared_ptr<std::string>>& objects);

    /**
     * Updates the cache with the contents from a commited
     * transaction. Any previously existing info will be
     * overwritten.
     */
    Error putTxDescriptor(const std::shared_ptr<AmTxDescriptor> txDesc, fds_uint64_t const blobSize);

    /**
     * Updates a blob descriptor in the cache for given volume id
     * and blob name. Any previously existing descriptor will be
     * overwritten.
     */
    Error putBlobDescriptor(fds_volid_t const volId,
                            typename descriptor_cache_type::key_type const& blobName,
                            typename descriptor_cache_type::value_type const blobDesc);

    Error putOffset(fds_volid_t const volId,
                    typename offset_cache_type::key_type const& blobOff,
                    typename offset_cache_type::value_type const objId);

    /**
     * Inserts new object into the object cache.
     */
    Error putObject(fds_volid_t const volId,
                    typename object_cache_type::key_type const& objId,
                    typename object_cache_type::value_type const obj);

    /**
     * Removes cache entries for a specific blob in a volume.
     */
    Error removeBlob(fds_volid_t volId, const std::string &blobName);

  private:
    descriptor_cache_type descriptor_cache;
    offset_cache_type offset_cache;
    object_cache_type object_cache;

    /// Max number of metadta entries per volume cache
    size_t max_metadata_entries;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_

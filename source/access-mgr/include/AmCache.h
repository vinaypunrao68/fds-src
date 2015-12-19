/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_

#include <string>

#include "AmAsyncDataApi.h"
#include "AmDataProvider.h"
#include <blob/BlobTypes.h>
#include <cache/VolumeSharedKvCache.h>

namespace fds {

struct AmTxDescriptor;
struct GetBlobReq;
struct GetObjectReq;
struct CommonModuleProviderIf;

/**
 * A client-side cache of blob metadata and data. The cache
 * multiplexes different volumes and allows for different
 * policy management for each volume.
 */
class AmCache :
    public AmDataProvider
{
    typedef VolumeSharedCacheManager<std::string, BlobDescriptor>
        descriptor_cache_type;
    typedef VolumeSharedCacheManager<BlobOffsetPair, ObjectID, BlobOffsetPairHash>
        offset_cache_type;
    typedef VolumeSharedCacheManager<ObjectID, std::string, ObjectHash, std::true_type>
        object_cache_type;

  public:
    AmCache(AmDataProvider* prev, CommonModuleProviderIf* modProvider);
    AmCache(AmCache const&) = delete;
    AmCache& operator=(AmCache const&) = delete;
    ~AmCache() override;

    /**
     * Updates the cache with the contents from a commited
     * transaction. Any previously existing info will be
     * overwritten.
     */
    Error putTxDescriptor(const std::shared_ptr<AmTxDescriptor> txDesc, fds_uint64_t const blobSize);

    /**
     * These are the Cache specific DataProvider routines.
     * Everything else is pass-thru.
     */
    bool done() override;
    void registerVolume(const VolumeDesc& volDesc) override;
    void removeVolume(const VolumeDesc& volDesc) override;
    void statBlob(AmRequest * amReq) override;
    void getBlob(AmRequest * amReq) override;

  protected:

    /**
     * These are the response we actually care about seeing the results of
     */
    void getBlobCb(AmRequest * amReq, Error const error) override;
    void getOffsetsCb(AmRequest * amReq, Error const error) override;
    void getObjectCb(AmRequest * amReq, Error const error) override;
    void deleteBlobCb(AmRequest * amReq, Error const error) override;
    void openVolumeCb(AmRequest * amReq, Error const error) override;
    void renameBlobCb(AmRequest * amReq, Error const error) override;
    void statBlobCb(AmRequest * amReq, Error const error) override;
    void volumeContentsCb(AmRequest * amReq, Error const error) override;

  private:
    descriptor_cache_type descriptor_cache;
    offset_cache_type offset_cache;
    object_cache_type object_cache;

    typedef std::unique_ptr<std::deque<GetObjectReq*>> queue_type;  // NOLINT
    std::unordered_map<ObjectID, queue_type, ObjectHash> obj_get_queue;
    std::mutex obj_get_lock;

    /// Cache maximums
    size_t max_volume_data;
    size_t max_metadata_entries;

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
    void getObjects(GetBlobReq* amReq);

    /**
     * Internal get object request handler
     */
    void getObject(GetBlobReq* blobReq,
                   ObjectID::ptr const& obj_id,
                   boost::shared_ptr<std::string>& buf);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_

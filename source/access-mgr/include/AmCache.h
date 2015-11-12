/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_

#include <string>

#include "AmAsyncDataApi.h"
#include <fds_table.h>
#include <fds_volume.h>
#include <blob/BlobTypes.h>
#include <cache/VolumeSharedKvCache.h>

namespace fds {

struct AmDispatcher;
struct AmTxDescriptor;
struct AmRequest;
struct GetBlobReq;
struct GetObjectReq;
struct CommonModuleProviderIf;

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
    explicit AmCache(CommonModuleProviderIf* modProvider);
    AmCache(AmCache const&) = delete;
    AmCache& operator=(AmCache const&) = delete;
    ~AmCache();

    /**
     * Initialize lower layers
     */
    using processor_cb_type = std::function<void(AmRequest*, Error const&)>;
    void init(processor_cb_type cb);

    /**
     * Creates cache structures for the volume described
     * in the volume descriptor.
     */
    Error registerVolume(fds_volid_t const vol_uuid, size_t const num_objs);

    /**
     * Removes volume cache for the volume.
     * Any dirty entries must be flushed back to persistent
     * storage prior to removal, otherwise they will be lost.
     */
    Error removeVolume(fds_volid_t const volId);

    /**
     * Updates the cache with the contents from a commited
     * transaction. Any previously existing info will be
     * overwritten.
     */
    Error putTxDescriptor(const std::shared_ptr<AmTxDescriptor> txDesc, fds_uint64_t const blobSize);

    /**
     * Removes cache entries for a specific blob in a volume.
     */
    Error removeBlob(fds_volid_t volId, const std::string &blobName);

    /** These are here as a pass-thru to dispatcher until we have stackable
     * interfaces */
    Error attachVolume(std::string const& volume_name);
    void openVolume(AmRequest *amReq);
    Error closeVolume(fds_volid_t vol_id, fds_int64_t token);
    void statVolume(AmRequest *amReq);
    void setVolumeMetadata(AmRequest *amReq);
    void getVolumeMetadata(AmRequest *amReq);
    void volumeContents(AmRequest *amReq);
    void startBlobTx(AmRequest *amReq);
    void commitBlobTx(AmRequest *amReq);
    void abortBlobTx(AmRequest *amReq);
    void statBlob(AmRequest *amReq);
    void setBlobMetadata(AmRequest *amReq);
    void deleteBlob(AmRequest *amReq);
    void renameBlob(AmRequest *amReq);
    void getBlob(AmRequest *amReq);
    void putObject(AmRequest *amReq);
    void putBlob(AmRequest *amReq);
    void putBlobOnce(AmRequest *amReq);
    bool getNoNetwork() const;
    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb);
    Error getDMT();
    Error getDLT();

  private:
    descriptor_cache_type descriptor_cache;
    offset_cache_type offset_cache;
    object_cache_type object_cache;

    processor_cb_type processor_cb;

    typedef std::unique_ptr<std::deque<GetObjectReq*>> queue_type;  // NOLINT
    std::unordered_map<ObjectID, queue_type, ObjectHash> obj_get_queue;
    std::mutex obj_get_lock;

    /// Max number of metadta entries per volume cache
    size_t max_metadata_entries;

    // Unique ptr to the dispatcher, this is common for all caches
    std::unique_ptr<AmDispatcher> dispatcher;

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
    void getObjectCb(ObjectID const obj_id, Error const& error);
    void getOffsetsCb(GetBlobReq* blobReq, Error const& error);
    void getBlobCb(AmRequest *amReq, Error const& error);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMCACHE_H_

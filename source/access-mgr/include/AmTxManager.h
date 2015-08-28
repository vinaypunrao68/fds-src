/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMTXMANAGER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMTXMANAGER_H_

#include <deque>
#include <string>
#include <fds_error.h>
#include <unordered_map>
#include <blob/BlobTypes.h>
#include <concurrency/RwLock.h>
#include <fdsp/dm_types_types.h>
#include "fds_volume.h"

namespace fds {

struct AmCache;
struct AmRequest;
struct AmTxDescriptor;
struct AmVolume;
struct AmVolumeAccessToken;
struct GetBlobReq;
struct GetObjectReq;

/**
 * Manages outstanding AM transactions. The transaction manager tracks which
 * are in progress, how long they have been in progress, and coordintates
 * completion (i.e., commit) of transactions.
 * TODO(Andrew): Add volume and blob name associativity into the interface
 * and indexing.
 */
struct AmTxManager {
    using descriptor_ptr_type = std::shared_ptr<AmTxDescriptor>;

 private:
    /// Maps a TxId with its descriptor
    typedef std::unordered_map<BlobTxId, descriptor_ptr_type, BlobTxIdHash> TxMap;
    TxMap txMap;

    /// RW lock to protect the map
    mutable fds_rwlock txMapLock;

    /// Maximum number of entries to stage
    fds_uint32_t maxStagedEntries;

    /// Maximum size of volume cache in bytes
    fds_uint32_t maxPerVolumeCacheSize;

    // Unique ptr to the data object cache
    std::unique_ptr<AmCache> amCache;

  public:
    AmTxManager();
    AmTxManager(AmTxManager const&) = delete;
    AmTxManager& operator=(AmTxManager const&) = delete;
    ~AmTxManager();

    /**
     * Initialize the cache and volume table and register
     * the callback we make to the transaction layer
     */
    using processor_cb_type = std::function<void(AmRequest*)>;
    void init(processor_cb_type cb);


    /**
     * Removes an existing transaction from the manager, destroying
     * any staged object updates. An error is returned if the transaction
     * ID does not already exist.
     */
    Error abortTx(const BlobTxId &txId);

    /**
     * Adds a new transaction to the manager. An error is returned
     * if the transaction ID exists already.
     * TODO(Andrew): Today we enforce transaction ID to be unique
     * across volumes.
     */
    Error addTx(fds_volid_t volId,
                const BlobTxId &txId,
                fds_uint64_t dmtVer,
                const std::string &name);

    /**
     * Notify that there is a newly attached volume, and build any
     * necessary data structures.
     */
    Error registerVolume(const VolumeDesc& volDesc, bool const can_cache_meta = false);

    /**
     * Remove all metadata caches
     */
    void invalidateMetaCache(const VolumeDesc& volDesc);
    void invalidateMetaCache(const fds_volid_t volId);

    /**
     * Notify that we have detached a volume, and remove any available
     * data structures.
     */
    Error removeVolume(const VolumeDesc& volDesc);

    /**
     * Removes the transaction and pushes all updates into the cache.
     */
    Error commitTx(const BlobTxId& txId, fds_uint64_t const blobSize);

    /**
     * Gets the DMT version for a given transaction ID. Returns an
     * error if the transaction ID does not already exist.
     */
    Error getTxDmtVersion(const BlobTxId &txId, fds_uint64_t *dmtVer) const;

    /**
     * Updates an existing transaction with a new operation
     */
    Error updateTxOpType(const BlobTxId &txId, fds_io_op_t op);

    /**
     * Updates an existing transactions staged blob offsets.
     */
    Error updateStagedBlobOffset(const BlobTxId &txId,
                                 const std::string &blobName,
                                 fds_uint64_t blobOffset,
                                 const ObjectID &objectId);

    /**
     * Updates an existing transactions staged blob objects.
     */
    Error updateStagedBlobObject(const BlobTxId &txId,
                                 const ObjectID &objectId,
                                 boost::shared_ptr<std::string> objectData);

    /**
     * Updates an existing transactions staged metadata.
     */
    Error updateStagedBlobDesc(const BlobTxId &txId,
                               fpi::FDSP_MetaDataList const& metaDataList);

    /**
     * Cache operations
     * TODO(bszmyd): Sun 22 Mar 2015 07:13:59 PM PDT
     * These are kinda ugly. When we do real transactions we should clean
     * this up.
     */
    BlobDescriptor::ptr getBlobDescriptor(fds_volid_t volId,
                                          std::string const& blobName,
                                          Error &error);
    Error putBlobDescriptor(fds_volid_t const volId,
                            std::string const& blobName,
                            boost::shared_ptr<BlobDescriptor> const blobDesc);

    Error getBlobOffsetObjects(fds_volid_t volId,
                               std::string const& blobName,
                               fds_uint64_t const obj_offset,
                               fds_uint64_t const obj_offset_end,
                               size_t const obj_size,
                               std::vector<ObjectID::ptr>& obj_ids);

    Error putOffsets(fds_volid_t const vol_id,
                     std::string const& blob_name,
                     fds_uint64_t const blob_offset,
                     fds_uint32_t const object_size,
                     std::vector<boost::shared_ptr<ObjectID>> const& object_ids);

    void getObjects(GetBlobReq* blobReq);

    Error removeBlob(fds_volid_t volId, const std::string &blobName);

  private:
    descriptor_ptr_type pop_descriptor(const BlobTxId& txId);
    processor_cb_type processor_enqueue;

    typedef std::unique_ptr<std::deque<GetObjectReq*>> queue_type;  // NOLINT
    std::unordered_map<ObjectID, queue_type, ObjectHash> obj_get_queue;
    std::mutex obj_get_lock;

    /**
     * Internal get object request handler
     */
    void getObject(GetBlobReq* blobReq,
                   ObjectID::ptr const& obj_id,
                   boost::shared_ptr<std::string>& buf);
    void getObjectCb(ObjectID const obj_id, Error const& error);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMTXMANAGER_H_

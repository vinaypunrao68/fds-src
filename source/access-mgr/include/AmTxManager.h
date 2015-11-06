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
#include "AmAsyncDataApi.h"
#include <fdsp/dm_types_types.h>
#include "fds_table.h"
#include "fds_volume.h"

namespace fds {

struct AmCache;
struct AmRequest;
struct AmTxDescriptor;
struct GetBlobReq;
class CommonModuleProviderIf;
class RandNumGenerator;

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

    /**
     * FEATURE TOGGLE: Safe UpdateBlobOnce
     * Wed 19 Aug 2015 10:56:46 AM MDT
     */
    bool safe_atomic_write { false };

  public:
    explicit AmTxManager(CommonModuleProviderIf *modProvider);
    AmTxManager(AmTxManager const&) = delete;
    AmTxManager& operator=(AmTxManager const&) = delete;
    ~AmTxManager();

    /**
     * Initialize the cache and volume table and register
     * the callback we make to the transaction layer
     */
    using processor_cb_type = std::function<void(AmRequest*, Error const&)>;
    void init(bool const safe_atomic, processor_cb_type cb);


    /**
     * Notify that there is a newly attached volume, and build any
     * necessary data structures.
     */
    Error registerVolume(const VolumeDesc& volDesc, bool const can_cache_meta = false);

    /**
     * Notify that we have detached a volume, and remove any available
     * data structures.
     */
    Error removeVolume(const fds_volid_t volId);

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
    void putBlob(AmRequest *amReq);
    bool getNoNetwork() const;
    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb);
    Error getDMT();
    Error getDLT();

  private:
    descriptor_ptr_type pop_descriptor(const BlobTxId& txId);
    processor_cb_type processor_cb;

    /// Unique ptr to a random num generator for tx IDs
    std::unique_ptr<RandNumGenerator> randNumGen;

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
     * Removes an existing transaction from the manager, destroying
     * any staged object updates. An error is returned if the transaction
     * ID does not already exist.
     */
    Error abortTx(const BlobTxId &txId);

    /**
     * Updates an existing transaction with a new operation
     */
    Error updateTxOpType(const BlobTxId &txId, fds_io_op_t op);

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
     * Updates an existing transactions staged blob objects.
     */
    Error updateStagedBlobObject(const BlobTxId &txId,
                                 const ObjectID &objectId,
                                 boost::shared_ptr<std::string> objectData);

    Error updateStagedBlobOffset(const BlobTxId &txId,
                                 const std::string &blobName,
                                 fds_uint64_t blobOffset,
                                 const ObjectID &objectId);

    Error updateStagedBlobDesc(const BlobTxId &txId,
                               fpi::FDSP_MetaDataList const& metaDataList);


    /**
     * Internal get object request handler
     */
    void putBlobCb(AmRequest *amReq, Error const& error);
    void putBlobOnceCb(AmRequest *amReq, Error const& error);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMTXMANAGER_H_

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMTXMANAGER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMTXMANAGER_H_

#include <string>
#include <unordered_map>

#include "AmAsyncDataApi.h"
#include "AmDataProvider.h"

#include <blob/BlobTypes.h>
#include <concurrency/RwLock.h>
#include <fdsp/dm_types_types.h>

namespace fds {

struct AmTxDescriptor;
struct PutBlobReq;
class RandNumGenerator;

/**
 * Manages outstanding AM transactions. The transaction manager tracks which
 * are in progress, how long they have been in progress, and coordintates
 * completion (i.e., commit) of transactions.
 * TODO(Andrew): Add volume and blob name associativity into the interface
 * and indexing.
 */
struct AmTxManager :
    public AmDataProvider
{
    using descriptor_ptr_type = std::shared_ptr<AmTxDescriptor>;

 private:
    /// Maps a TxId with its descriptor
    typedef std::unordered_map<BlobTxId, descriptor_ptr_type, BlobTxIdHash> TxMap;
    TxMap txMap;

    /// RW lock to protect the map
    mutable fds_rwlock txMapLock;

    /// Maximum number of entries to stage
    fds_uint32_t maxStagedEntries;

  public:
    explicit AmTxManager(AmDataProvider* prev);
    AmTxManager(AmTxManager const&) = delete;
    AmTxManager& operator=(AmTxManager const&) = delete;
    ~AmTxManager() override;

    /**
     * These are the Transaction specific DataProvider routines.
     * Everything else is pass-thru.
     */
    void start() override;
    void startBlobTx(AmRequest *amReq) override;
    void commitBlobTx(AmRequest *amReq) override;
    void abortBlobTx(AmRequest *amReq) override;
    void setBlobMetadata(AmRequest *amReq) override;
    void deleteBlob(AmRequest *amReq) override;
    void renameBlob(AmRequest *amReq) override;
    void getBlob(AmRequest *amReq) override;
    void putBlob(AmRequest *amReq) override;
    void putBlobOnce(AmRequest *amReq) override;

  protected:

    /**
     * These are the response we actually care about seeing the results of
     */
    void commitBlobTxCb(AmRequest * amReq, Error const error) override;
    void startBlobTxCb(AmRequest * amReq, Error const error) override;
    void setBlobMetadataCb(AmRequest * AmReq, Error const error) override;
    void deleteBlobCb(AmRequest * amReq, Error const error) override;
    void renameBlobCb(AmRequest * amReq, Error const error) override;
    void putObjectCb(AmRequest * amReq, Error const error) override;
    void updateCatalogCb(AmRequest * amReq, Error const error) override;

  private:
    descriptor_ptr_type pop_descriptor(const BlobTxId& txId);

    /// Unique ptr to a random num generator for tx IDs
    std::unique_ptr<RandNumGenerator> randNumGen;

    /**
     * FEATURE TOGGLE: All atomic OPs toggle
     * Wed Jan 20 18:59:22 2016
     */
    bool all_atomic_ops { false };

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

    void applyPut(PutBlobReq* blobReq);

    /**
     * Removes an existing transaction from the manager, destroying
     * any staged object updates. An error is returned if the transaction
     * ID does not already exist.
     */
    Error abortTx(const BlobTxId &txId);
    void abortOnError(AmRequest *amReq, Error const error);

    /**
     * Updates an existing transaction with a new operation
     */
    Error updateTxOpType(const BlobTxId &txId, fds_io_op_t op);

    void _putBlobCb(AmRequest * amReq, Error const error);

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
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMTXMANAGER_H_

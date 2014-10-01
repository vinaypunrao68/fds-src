/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AM_TX_MGR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AM_TX_MGR_H_

#include <map>
#include <string>
#include <fds_module.h>
#include <fds_error.h>
#include <unordered_map>
#include <blob/BlobTypes.h>
#include <concurrency/RwLock.h>

namespace fds {

/**
 * Descriptor for an AM blob transaction. Contains information
 * about the transaction and a staging area for the updates.
 */
class AmTxDescriptor {
  public:
    /// Blob trans ID
    BlobTxId txId;
    /// DMT version when transaction started
    fds_uint64_t originDmtVersion;
    /// Blob being mutated
    std::string blobName;
    /// Volume context for the transaction
    fds_volid_t volId;

    /// Type of the staged transaction
    fds_io_op_t opType;
    /// Staged blob descriptor for the transaction
    BlobDescriptor::ptr stagedBlobDesc;
    /// Staged blob offsets updates
    typedef std::unordered_map<BlobOffsetPair, ObjectID,
                               BlobOffsetPairHash> BlobOffsetMap;
    BlobOffsetMap stagedBlobOffsets;
    /// Staged blob object updates for the transaction
    typedef std::unordered_map<ObjectID, std::string*,
                               ObjectHash> BlobObjectMap;
    BlobObjectMap stagedBlobObjects;

    AmTxDescriptor(fds_volid_t volUuid,
                   const BlobTxId &id,
                   fds_uint64_t dmtVer,
                   const std::string &name);
    ~AmTxDescriptor();
    typedef boost::shared_ptr<AmTxDescriptor> ptr;
};

/**
 * Manages outstanding AM transactions. The transaction manager tracks which
 * are in progress, how long they have been in progress, and coordintates
 * completion (i.e., commit) of transactions.
 * TODO(Andrew): Add volume and blob name associativity into the interface
 * and indexing.
 */
class AmTxManager : public Module, public boost::noncopyable {
  private:
    /// Maps a TxId with its descriptor
    typedef std::unordered_map<BlobTxId, AmTxDescriptor::ptr, BlobTxIdHash> TxMap;
    TxMap txMap;

    /// RW lock to protect the map
    mutable fds_rwlock txMapLock;

    /// Maximum number of entries to stage
    fds_uint32_t maxStagedEntries;

  public:
    explicit AmTxManager(const std::string &modName);
    ~AmTxManager();
    typedef std::unique_ptr<AmTxManager> unique_ptr;

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

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
     * Removes an existing transaction from the manager. An error is
     * returned if the transaction ID does not already exist.
     */
    Error removeTx(const BlobTxId &txId);

    /**
     * Returns the descriptor associated with the transaction.
     * Note the memory returned is still owned by the manager
     * and may still be modified or removed.
     */
    Error getTxDescriptor(const BlobTxId &txId, AmTxDescriptor::ptr &desc);

    /**
     * Gets the DMT version for a given transaction ID. Returns an
     * error if the transaction ID does not already exist.
     */
    Error getTxDmtVersion(const BlobTxId &txId, fds_uint64_t *dmtVer) const;

    /**
     * Updates an existing transaction with a new ope
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
                                 const char *objectData,
                                 fds_uint32_t dataLen);

    /**
     * Updates an existing transactions staged metadata.
     */
    Error updateStagedBlobDesc(const BlobTxId &txId,
                               boost::shared_ptr<fpi::FDSP_MetaDataList> metaDataList);

    /**
     * Updates an existing transactions staged metadata.
     */
    Error updateStagedBlobDesc(const BlobTxId &txId,
                               boost::shared_ptr<
                               std::map<std::string, std::string>> &metadata);

    /**
     * Updates an existing transactions staged update length.
     * TODO(Andrew): This assumes that the transaction is going
     * update the entire blob and that each offset update is unique.
     * If this assumption is not true, the cached blob size will be incorrect.
     */
    Error updateStagedBlobDesc(const BlobTxId &txId,
                               fds_uint64_t len);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AM_TX_MGR_H_

/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */
#include "AmTxManager.h"

#include <map>
#include <string>
#include <fds_process.h>
#include "AmCache.h"
#include "AmTxDescriptor.h"
#include "FdsRandom.h"
#include <ObjectId.h>
#include "requests/requests.h"

namespace fds {

AmTxManager::AmTxManager(AmDataProvider* prev, CommonModuleProviderIf *modProvider)
    : AmDataProvider(prev, new AmCache(this, modProvider))
{
}

AmTxManager::~AmTxManager() = default;

void AmTxManager::start() {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    maxStagedEntries = conf.get<fds_uint32_t>("cache.tx_max_staged_entries");

    randNumGen = RandNumGenerator::unique_ptr(
        new RandNumGenerator(RandNumGenerator::getRandSeed()));

    AmDataProvider::start();
}

Error
AmTxManager::addTx(fds_volid_t volId,
                   const BlobTxId &txId,
                   fds_uint64_t dmtVer,
                   const std::string &name) {
    SCOPEDWRITE(txMapLock);
    if (txMap.count(txId) > 0) {
        return ERR_DUPLICATE_UUID;
    }
    txMap[txId] = std::make_shared<AmTxDescriptor>(volId, txId, dmtVer, name);

    return ERR_OK;
}

AmTxManager::descriptor_ptr_type
AmTxManager::pop_descriptor(const BlobTxId &txId) {
    SCOPEDWRITE(txMapLock);
    descriptor_ptr_type ret_val = nullptr;
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt != txMap.end()) {
        ret_val = txMapIt->second;
        txMap.erase(txMapIt);
    }
    return ret_val;
}

Error
AmTxManager::abortTx(const BlobTxId &txId) {
    return pop_descriptor(txId) ? ERR_OK : ERR_NOT_FOUND;
}

Error
AmTxManager::getTxDmtVersion(const BlobTxId &txId, fds_uint64_t *dmtVer) const {
    SCOPEDREAD(txMapLock);
    TxMap::const_iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    *dmtVer = txMapIt->second->originDmtVersion;
    return ERR_OK;
}

Error
AmTxManager::updateTxOpType(const BlobTxId &txId,
                            fds_io_op_t op) {
    SCOPEDWRITE(txMapLock);
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    fds_verify(txId == txMapIt->second->txId);
    // TODO(Andrew): For now, we're only expecting to change
    // from PUT to DELETE
    fds_verify(txMapIt->second->opType == FDS_PUT_BLOB);
    fds_verify(op == FDS_DELETE_BLOB);
    txMapIt->second->opType = op;

    return ERR_OK;
}

Error
AmTxManager::updateStagedBlobOffset(const BlobTxId &txId,
                                    const std::string &blobName,
                                    fds_uint64_t blobOffset,
                                    const ObjectID &objectId) {
    SCOPEDREAD(txMapLock);
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    fds_verify(txId == txMapIt->second->txId);
    // TODO(Andrew): For now, we're only expecting to update
    // a PUT transaction
    fds_verify(txMapIt->second->opType == FDS_PUT_BLOB);

    BlobOffsetPair offsetPair(blobName, blobOffset);
    fds_verify(txMapIt->second->stagedBlobOffsets.count(offsetPair) == 0);

    if (txMapIt->second->stagedBlobOffsets.size() < maxStagedEntries) {
        txMapIt->second->stagedBlobOffsets[offsetPair] = objectId;
    }

    return ERR_OK;
}

Error
AmTxManager::updateStagedBlobObject(const BlobTxId &txId,
                                    const ObjectID &objectId,
                                    boost::shared_ptr<std::string> objectData) {
    SCOPEDREAD(txMapLock);
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    fds_verify(txId == txMapIt->second->txId);
    // TODO(Andrew): For now, we're only expecting to update
    // a PUT transaction
    fds_verify(txMapIt->second->opType == FDS_PUT_BLOB);

    if (txMapIt->second->stagedBlobObjects.size() < maxStagedEntries) {
        // Copy the data into the tx manager. This allows the
        // tx manager to hand off the ptr to the cache later.
        txMapIt->second->stagedBlobObjects[objectId] =
                objectData;
    }

    return ERR_OK;
}

Error
AmTxManager::updateStagedBlobDesc(const BlobTxId &txId,
                                  fpi::FDSP_MetaDataList const& metaDataList) {
    SCOPEDREAD(txMapLock);
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    fds_verify(txId == txMapIt->second->txId);
    // Verify that we're not overwriting previously staged metadata
    fds_verify(txMapIt->second->stagedBlobDesc->kvMetaBegin() ==
               txMapIt->second->stagedBlobDesc->kvMetaEnd());

    for (auto const & meta : metaDataList) {
        txMapIt->second->stagedBlobDesc->addKvMeta(meta.key, meta.value);
    }
    return ERR_OK;
}

Error
AmTxManager::commitTx(const BlobTxId &txId, fds_uint64_t const blobSize)
{
    if (auto descriptor = pop_descriptor(txId)) {
        return static_cast<AmCache*>(getNextInChain())->putTxDescriptor(descriptor, blobSize);
    }
    return ERR_NOT_FOUND;
}

void
AmTxManager::startBlobTx(AmRequest *amReq) {
    auto blobReq = static_cast<StartBlobTxReq*>(amReq);
    // Generate a random transaction ID to use
    blobReq->tx_desc = boost::make_shared<BlobTxId>(randNumGen->genNumSafe());
    AmDataProvider::startBlobTx(amReq);
}

void
AmTxManager::startBlobTxCb(AmRequest * amReq, Error const error) {
    auto blobReq = static_cast<StartBlobTxReq*>(amReq);
    StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback, blobReq->cb);
    auto err = error;
    if (err.ok()) {
        // Update callback and record new open transaction
        cb->blobTxId = *blobReq->tx_desc;
        err = addTx(blobReq->io_vol_id,
                    *blobReq->tx_desc,
                    blobReq->dmt_version,
                    blobReq->getBlobName());
    }
    AmDataProvider::startBlobTxCb(blobReq, err);
}

void
AmTxManager::commitBlobTx(AmRequest *amReq) {
    auto blobReq = static_cast<CommitBlobTxReq*>(amReq);
    auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        return AmDataProvider::commitBlobTxCb(amReq, err);
    }
    AmDataProvider::commitBlobTx(amReq);
}

void
AmTxManager::commitBlobTxCb(AmRequest * amReq, Error const error) {
    auto blobReq = static_cast<CommitBlobTxReq*>(amReq);
    // Push the committed update to the cache and remove from manager
    if (ERR_OK == error) {
        updateStagedBlobDesc(*(blobReq->tx_desc), blobReq->final_meta_data);
        commitTx(*(blobReq->tx_desc), blobReq->final_blob_size);
    } else {
        LOGERROR << "Transaction failed to commit: " << error;
    }
    AmDataProvider::commitBlobTxCb(blobReq, error);
}

void
AmTxManager::abortBlobTx(AmRequest *amReq) {
    auto blobReq = static_cast<AbortBlobTxReq*>(amReq);
    auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        return AmDataProvider::abortBlobTxCb(amReq, err);
    }
    AmDataProvider::abortBlobTx(amReq);
}

void
AmTxManager::abortBlobTxCb(AmRequest * amReq, Error const error) {
    auto blobReq = static_cast<AbortBlobTxReq*>(amReq);
    if (ERR_OK != abortTx(*(blobReq->tx_desc))) {
        LOGWARN << "Transaction unknown";
    }
    AmDataProvider::abortBlobTxCb(blobReq, error);
}

void
AmTxManager::setBlobMetadata(AmRequest *amReq) {
    auto blobReq = static_cast<SetBlobMetaDataReq*>(amReq);
    // Assert that we have a tx and dmt_version for the message
    auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        return AmDataProvider::setBlobMetadataCb(amReq, err);
    }
    AmDataProvider::setBlobMetadata(amReq);
}

void
AmTxManager::deleteBlob(AmRequest *amReq) {
    auto blobReq = static_cast<DeleteBlobReq *>(amReq);
    LOGDEBUG    << " volume:" << amReq->io_vol_id
                << " blob:" << amReq->getBlobName()
                << " txn:" << blobReq->tx_desc;

    // Update the tx manager with the delete op
    auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        return AmDataProvider::deleteBlobCb(amReq, err);
    }
    updateTxOpType(*(blobReq->tx_desc), amReq->io_type);
    AmDataProvider::deleteBlob(amReq);
}

void
AmTxManager::renameBlob(AmRequest *amReq) {
    auto blobReq = static_cast<RenameBlobReq*>(amReq);
    LOGDEBUG << "Renaming blob: " << blobReq->getBlobName()
             << " to: " << blobReq->new_blob_name;
    blobReq->tx_desc.reset(new BlobTxId(randNumGen->genNumSafe()));
    blobReq->dest_tx_desc.reset(new BlobTxId(randNumGen->genNumSafe()));
    AmDataProvider::renameBlob(amReq);
}

void
AmTxManager::getBlob(AmRequest *amReq) {
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);
    blobReq->blob_offset = (amReq->blob_offset * blobReq->object_size);
    blobReq->blob_offset_end = blobReq->blob_offset;

    // If this is a large read, the number of end offset needs to encompass
    // the extra objects required.
    amReq->object_size = blobReq->object_size;
    if (blobReq->object_size < amReq->data_len) {
        auto const extra_objects = (amReq->data_len / blobReq->object_size) - 1
                                 + ((0 != amReq->data_len % blobReq->object_size) ? 1 : 0);
        blobReq->blob_offset_end += extra_objects * blobReq->object_size;
    }

    // Create buffers for return objects, we don't know how many till we have
    // a valid descriptor
    GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, amReq->cb);
    cb->return_buffers = boost::make_shared<std::vector<boost::shared_ptr<std::string>>>();

    // FIXME(bszmyd): Sun 26 Apr 2015 04:41:12 AM MDT
    // Don't support unaligned currently, reject if this is not
    if (0 != (amReq->blob_offset % blobReq->object_size)) {
        LOGERROR << "unaligned read not supported, offset: " << amReq->blob_offset
                 << " length: " << amReq->data_len;
        return AmDataProvider::getBlobCb(amReq, ERR_INVALID);
    }

    AmDataProvider::getBlob(amReq);
}

void
AmTxManager::putBlob(AmRequest *amReq) {
    // Use a stock object ID if the length is 0.
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(amReq);
    blobReq->blob_offset = (blobReq->blob_offset * blobReq->object_size);
    if (amReq->data_len == 0) {
        blobReq->obj_id = ObjectID();
    } else {
        SCOPED_PERF_TRACEPOINT_CTX(amReq->hash_perf_ctx);
        blobReq->obj_id = ObjIdGen::genObjectId(blobReq->dataPtr->c_str(), amReq->data_len);
    }

    // Verify we have a dmt version (and transaction) for this update
    auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        return AmDataProvider::putBlobCb(amReq, err);
    }
    AmDataProvider::putBlob(amReq);
}

void
AmTxManager::putBlobOnce(AmRequest *amReq) {
    // Use a stock object ID if the length is 0.
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(amReq);
    blobReq->blob_offset = (blobReq->blob_offset * blobReq->object_size);
    if (amReq->data_len == 0) {
        blobReq->obj_id = ObjectID();
    } else {
        SCOPED_PERF_TRACEPOINT_CTX(amReq->hash_perf_ctx);
        blobReq->obj_id = ObjIdGen::genObjectId(blobReq->dataPtr->c_str(), amReq->data_len);
    }

    blobReq->setTxId(randNumGen->genNumSafe());
    return AmDataProvider::putBlobOnce(amReq);
}

void
AmTxManager::applyPut(PutBlobReq* blobReq) {
    auto tx_desc = blobReq->tx_desc;
    // Update the transaction manager with the stage offset update
    if (ERR_OK != updateStagedBlobOffset(*tx_desc,
                                         blobReq->getBlobName(),
                                         blobReq->blob_offset,
                                         blobReq->obj_id)) {
        // An abort or commit already caused the tx
        // to be cleaned up. Short-circuit
        GLOGNOTIFY << "Response no longer has active transaction: " << tx_desc->getValue();
        return;
    }

    // Update the transaction manager with the staged object data
    if (blobReq->data_len > 0) {
        updateStagedBlobObject(*tx_desc, blobReq->obj_id, blobReq->dataPtr);
    }
}

void
AmTxManager::putBlobCb(AmRequest* amReq, Error const error) {
    if (error.ok()) {
        applyPut(static_cast<PutBlobReq *>(amReq));
    }
    AmDataProvider::putBlobCb(amReq, error);
}

void
AmTxManager::putBlobOnceCb(AmRequest* amReq, Error const error) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    if (error.ok()) {
        auto tx_desc = blobReq->tx_desc;
        // Add the Tx to the manager if this an updateOnce
        if (ERR_OK == addTx(amReq->io_vol_id,
                            *tx_desc,
                            blobReq->dmt_version,
                            amReq->getBlobName())) {
            // Stage the transaction metadata changes
            updateStagedBlobDesc(*tx_desc, blobReq->final_meta_data);
            applyPut(blobReq);
            commitTx(*tx_desc, blobReq->final_blob_size);
        }
    }
    AmDataProvider::putBlobOnceCb(amReq, error);
}

}  // namespace fds

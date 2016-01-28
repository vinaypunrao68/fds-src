/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */
#include "AmTxManager.h"

#include <fds_process.h>
#include "AmCache.h"
#include "AmTxDescriptor.h"
#include "FdsRandom.h"
#include <ObjectId.h>
#include "requests/AbortBlobTxReq.h"
#include "requests/CommitBlobTxReq.h"
#include "requests/DeleteBlobReq.h"
#include "requests/GetBlobReq.h"
#include "requests/PutBlobReq.h"
#include "requests/PutObjectReq.h"
#include "requests/RenameBlobReq.h"
#include "requests/SetBlobMetaDataReq.h"
#include "requests/StartBlobTxReq.h"
#include "requests/UpdateCatalogReq.h"

namespace fds {

AmTxManager::AmTxManager(AmDataProvider* prev)
    : AmDataProvider(prev, new AmCache(this))
{
}

AmTxManager::~AmTxManager() = default;

void AmTxManager::start() {
    /**
     * FEATURE TOGGLE: All Atomic ops switch
     * Wed Jan 20 18:59:47 2016
     */
    FdsConfigAccessor ft_conf(g_fdsprocess->get_fds_config(), "fds.feature_toggle.");
    all_atomic_ops = ft_conf.get<bool>("am.all_atomic_ops", all_atomic_ops);

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
                   const std::string &name,
                   const fds_int32_t blob_mode) {
    SCOPEDWRITE(txMapLock);
    if (txMap.count(txId) > 0) {
        return ERR_DUPLICATE_UUID;
    }
    txMap[txId] = std::make_shared<AmTxDescriptor>(volId, txId, dmtVer, name, blob_mode);

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
                                    const ObjectID &objectId,
                                    size_t const length) {
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
        txMapIt->second->stagedBlobOffsets[offsetPair] = std::make_pair(objectId, length);
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

void
AmTxManager::abortOnError(AmRequest *amReq, Error const error) {
    // If this failed, implicitly abort the tx, Xdi won't
    if (ERR_OK != error) {
        auto txReq = dynamic_cast<AmTxReq*>(amReq);
        auto abort_req = new AbortBlobTxReq(amReq->io_vol_id,
                                            amReq->volume_name,
                                            amReq->getBlobName(),
                                            txReq->tx_desc,
                                            nullptr);
        abortBlobTx(abort_req);
    }
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
    if (!all_atomic_ops) {
        AmDataProvider::startBlobTx(amReq);
        return;
    }
    startBlobTxCb(blobReq, ERR_OK);
}

void
AmTxManager::startBlobTxCb(AmRequest * amReq, Error const error) {
    auto blobReq = static_cast<StartBlobTxReq*>(amReq);
    auto err = error;
    if (err.ok()) {
        // Update callback and record new open transaction
        if (blobReq->cb) {
            auto cb = std::dynamic_pointer_cast<StartBlobTxCallback>(blobReq->cb);
            cb->blobTxId = *blobReq->tx_desc;
        }
        if (blobReq->delete_request) {
            // XXX (bszmyd):  This is a hack until delete is atomic Fri Jan 22 18:49:01 2016
            {
                SCOPEDREAD(txMapLock);
                txMap[*blobReq->tx_desc]->originDmtVersion = blobReq->dmt_version;
            }
            auto delReq = static_cast<DeleteBlobReq*>(blobReq->delete_request);
            delReq->dmt_version = blobReq->dmt_version;
            updateTxOpType(*(blobReq->tx_desc), delReq->io_type);
            AmDataProvider::deleteBlob(delReq);
        } else {
            err = addTx(blobReq->io_vol_id,
                        *blobReq->tx_desc,
                        blobReq->dmt_version,
                        blobReq->getBlobName(),
                        blobReq->blob_mode);
        }
    }
    AmDataProvider::startBlobTxCb(blobReq, err);
}

void
AmTxManager::commitBlobTx(AmRequest *amReq) {
    auto blobReq = static_cast<CommitBlobTxReq*>(amReq);
    auto const& txId = *blobReq->tx_desc;
    Error err = getTxDmtVersion(txId, &(blobReq->dmt_version));
    if (!err.ok()) {
        AmDataProvider::commitBlobTxCb(blobReq, err);
    }

    SCOPEDREAD(txMapLock);
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        AmDataProvider::commitBlobTxCb(blobReq, ERR_INVALID_ARG);
        return;
    }
    blobReq->is_delete = (FDS_DELETE_BLOB == txMapIt->second->opType);

    if (!all_atomic_ops) {
        AmDataProvider::commitBlobTx(blobReq);
    } else {
        if (!blobReq->is_delete) {
            auto catUpdateReq = new UpdateCatalogReq(blobReq);
            catUpdateReq->blob_mode = txMapIt->second->blob_mode;
            for (auto const& offset_pair : txMapIt->second->stagedBlobOffsets) {
                auto const& obj_id = offset_pair.second.first;
                auto const data_length = offset_pair.second.second;
                catUpdateReq->object_list.emplace(obj_id,
                                   std::make_pair(offset_pair.first.getOffset(), data_length));
            }

            for (auto kvIt = txMapIt->second->stagedBlobDesc->kvMetaBegin();
                 txMapIt->second->stagedBlobDesc->kvMetaEnd() != kvIt;
                 ++kvIt) {
                catUpdateReq->metadata->emplace(kvIt->first, kvIt->second);
            }
            AmDataProvider::updateCatalog(catUpdateReq);
        } else {
            AmDataProvider::commitBlobTx(blobReq);
        }
    }
}

void
AmTxManager::commitBlobTxCb(AmRequest * amReq, Error const error) {
    auto blobReq = static_cast<CommitBlobTxReq*>(amReq);
    // Push the committed update to the cache and remove from manager
    if (error.ok()) {
        // Already written via putObjectCb...updateCatalogCb...
        if (!all_atomic_ops) {
            updateStagedBlobDesc(*(blobReq->tx_desc), blobReq->final_meta_data);
            commitTx(*(blobReq->tx_desc), blobReq->final_blob_size);
        }
    } else {
        LOGERROR << "Transaction failed to commit: " << error;
        abortOnError(blobReq, error);
    }
    AmDataProvider::commitBlobTxCb(blobReq, error);
}

void
AmTxManager::abortBlobTx(AmRequest *amReq) {
    auto blobReq = static_cast<AbortBlobTxReq*>(amReq);
    auto const& tx_desc = *(blobReq->tx_desc);
    Error err { ERR_OK };
    pop_descriptor(tx_desc);
    if (!all_atomic_ops) {
        err = getTxDmtVersion(tx_desc, &(blobReq->dmt_version));
        if (err.ok()) {
            AmDataProvider::abortBlobTx(blobReq);
            return;
        }
    }
    AmDataProvider::abortBlobTxCb(amReq, err);
}

void
AmTxManager::setBlobMetadata(AmRequest *amReq) {
    auto blobReq = static_cast<SetBlobMetaDataReq*>(amReq);
    Error err { ERR_OK };
    if (!all_atomic_ops) {
        auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
        // Assert that we have a tx and dmt_version for the message
        if (err.ok()) {
            AmDataProvider::setBlobMetadata(amReq);
            return;
        }
    } else {
        updateStagedBlobDesc(*(blobReq->tx_desc), *blobReq->metaDataList);
    }
    AmTxManager::setBlobMetadataCb(amReq, err);
}

void
AmTxManager::setBlobMetadataCb(AmRequest *amReq, Error const error) {
    abortOnError(amReq, error);
    AmDataProvider::setBlobMetadataCb(amReq, error);
}

void
AmTxManager::deleteBlob(AmRequest *amReq) {
    auto blobReq = static_cast<DeleteBlobReq *>(amReq);
    LOGDEBUG    << " volume:" << amReq->io_vol_id
                << " blob:" << amReq->getBlobName()
                << " txn:" << blobReq->tx_desc;

    if (!all_atomic_ops) {
        // Update the tx manager with the delete op
        auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
        if (err.ok()) {
            updateTxOpType(*(blobReq->tx_desc), amReq->io_type);
            AmDataProvider::deleteBlob(amReq);
        } else {
            AmDataProvider::deleteBlobCb(amReq, err);
        }
    } else {
        auto startReq = new StartBlobTxReq(blobReq->io_vol_id,
                                           blobReq->volume_name,
                                           blobReq->getBlobName(),
                                           0,
                                           nullptr);
        // XXX (bszmyd):  Delete needs to be _real_ Tx for now Fri Jan 22 18:47:13 2016
        startReq->tx_desc = blobReq->tx_desc;
        startReq->delete_request = amReq;
        AmDataProvider::startBlobTx(startReq);
    }
}

void
AmTxManager::deleteBlobCb(AmRequest *amReq, Error const error) {
    abortOnError(amReq, error);
    AmDataProvider::deleteBlobCb(amReq, error);
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
AmTxManager::renameBlobCb(AmRequest *amReq, Error const error) {
    abortOnError(amReq, error);
    AmDataProvider::renameBlobCb(amReq, error);
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
    auto cb = std::dynamic_pointer_cast<GetObjectCallback>(amReq->cb);
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
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    blobReq->blob_offset = (blobReq->blob_offset * blobReq->object_size);
    if (amReq->data_len == 0) {
        blobReq->obj_id = ObjectID();
    } else {
        SCOPED_PERF_TRACEPOINT_CTX(amReq->hash_perf_ctx);
        blobReq->obj_id = ObjIdGen::genObjectId(blobReq->dataPtr->c_str(), amReq->data_len);
    }

    // Create the request to update SM with the new object
    auto objReq = new PutObjectReq(blobReq);

    /**
     * FEATURE TOGGLE: All Atomic Ops
     * Wed Jan 20 19:05:42 2016
     * Instead of writing the DM update, just store the object if needed
     */
    if (!all_atomic_ops) {
        // stage a tx update in DM
        auto catUpdateReq = new UpdateCatalogReq(blobReq, false);
        catUpdateReq->object_list.emplace(blobReq->obj_id,
                                          std::make_pair(blobReq->blob_offset, blobReq->data_len));
        // Verify we have a dmt version (and transaction) for this update
        auto err = getTxDmtVersion(*(blobReq->tx_desc), &(catUpdateReq->dmt_version));
        if (!err.ok()) {
            delete catUpdateReq;
            AmDataProvider::putBlobCb(blobReq, err);
            return;
        }
        AmDataProvider::updateCatalog(catUpdateReq);
    } else {
        // Fake the DM write for now
        blobReq->notifyResponse(ERR_OK);
    }
    AmDataProvider::putObject(objReq);
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
    auto objReq = new PutObjectReq(blobReq);
    AmDataProvider::putObject(objReq);
}

void
AmTxManager::applyPut(PutBlobReq* blobReq) {
    auto const& tx_desc = *blobReq->tx_desc;
    // Update the transaction manager with the stage offset update
    if (ERR_OK != updateStagedBlobOffset(tx_desc,
                                         blobReq->getBlobName(),
                                         blobReq->blob_offset,
                                         blobReq->obj_id,
                                         blobReq->data_len)) {
        // An abort or commit already caused the tx
        // to be cleaned up. Short-circuit
        GLOGNOTIFY << "Response no longer has active transaction: " << tx_desc.getValue();
        return;
    }

    // Update the transaction manager with the staged object data
    if (blobReq->data_len > 0) {
        updateStagedBlobObject(tx_desc, blobReq->obj_id, blobReq->dataPtr);
    }
}

void
AmTxManager::putObjectCb(AmRequest *amReq, Error const error) {
    auto objReq = static_cast<PutObjectReq*>(amReq);
    auto blobReq = objReq->parent;
    delete objReq;
    auto err = error;
    if (fds::FDS_PUT_BLOB == blobReq->io_type) {
        _putBlobCb(blobReq, err);
        return;
    }
    if (err.ok()) {
        // Commit the change to DM
        auto catUpdateReq = new UpdateCatalogReq(blobReq);
        catUpdateReq->object_list.emplace(blobReq->obj_id,
                                          std::make_pair(blobReq->blob_offset, blobReq->data_len));
        AmDataProvider::updateCatalog(catUpdateReq);
        return;
    }
    AmDataProvider::putBlobOnceCb(blobReq, err);
}

void
AmTxManager::_putBlobCb(AmRequest* amReq, Error const error) {
    auto blobReq = static_cast<PutBlobReq*>(amReq);
    auto const& tx_desc = *blobReq->tx_desc;
    Error err {ERR_OK};
    bool done {true};
    if (!all_atomic_ops) {
        err = error;
        std::tie(done, err) = blobReq->notifyResponse(err);
        if (done) {
            if (err.ok()) {
                applyPut(blobReq);
            } else {
                abortOnError(blobReq, error);
            }
        }
    } else {
        err = updateStagedBlobOffset(tx_desc,
                                     blobReq->getBlobName(),
                                     blobReq->blob_offset,
                                     blobReq->obj_id,
                                     blobReq->data_len);
    }
    if (done) {
        AmDataProvider::putBlobCb(blobReq, error);
    }
}

void
AmTxManager::updateCatalogCb(AmRequest* amReq, Error const error) {
    auto catReq = static_cast<UpdateCatalogReq *>(amReq);
    auto blobReq = catReq->parent;
    delete catReq;

    if (fds::FDS_PUT_BLOB == blobReq->io_type) {
        _putBlobCb(blobReq, error);
        return;
    } else if (fds::FDS_COMMIT_BLOB_TX == blobReq->io_type) {
        commitBlobTxCb(blobReq, error);
        return;
    }
    AmDataProvider::putBlobOnceCb(blobReq, error);
}

}  // namespace fds

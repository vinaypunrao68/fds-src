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

static constexpr fds_uint32_t Ki { 1024 };
static constexpr fds_uint32_t Mi { 1024 * Ki };

AmTxManager::AmTxManager(CommonModuleProviderIf *modProvider)
    : amCache(new AmCache(modProvider))
{
}

AmTxManager::~AmTxManager() = default;

void AmTxManager::init(bool const safe_atomic, processor_cb_type cb) {
    // This determines if we can or cannot dispatch updatecatlog and putobject
    // concurrently.
    safe_atomic_write = safe_atomic;

    // This funtion is used to respond to requests
    processor_cb = cb;

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    maxStagedEntries = conf.get<fds_uint32_t>("cache.tx_max_staged_entries");
    // This is in terms of MiB
    maxPerVolumeCacheSize = Mi * conf.get<fds_uint32_t>("cache.max_volume_data");

    randNumGen = RandNumGenerator::unique_ptr(
        new RandNumGenerator(RandNumGenerator::getRandSeed()));

    amCache->init(processor_cb);
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
AmTxManager::registerVolume(const VolumeDesc& volDesc)
{
    // The cache size is controlled in terms of MiB, but the LRU
    // knows only terms in # of elements. Do this conversion.
    auto num_cached_objs = (0 < volDesc.maxObjSizeInBytes) ?
        (maxPerVolumeCacheSize / volDesc.maxObjSizeInBytes) : 0;

    // A duplicate is ok, renewal's of a token cause this
    auto err = amCache->registerVolume(volDesc.volUUID, num_cached_objs);
    switch (err.GetErrno()) {
        case ERR_OK:
            LOGDEBUG << "Created caches for volume: " << std::hex << volDesc.volUUID;
            break;;
        case ERR_VOL_DUPLICATE:
            err = ERR_OK;
            break;;
        default:
            LOGERROR << "Failed to register volume: " << err;
            break;;
    }

    return err;
}

Error
AmTxManager::removeVolume(const fds_volid_t volId)
{ return amCache->removeVolume(volId); }

Error
AmTxManager::commitTx(const BlobTxId &txId, fds_uint64_t const blobSize)
{
    if (auto descriptor = pop_descriptor(txId)) {
        return amCache->putTxDescriptor(descriptor, blobSize);
    }
    return ERR_NOT_FOUND;
}

Error
AmTxManager::attachVolume(std::string const& volume_name) {
    return amCache->attachVolume(volume_name);
}

void
AmTxManager::openVolume(AmRequest *amReq) {
    return amCache->openVolume(amReq);
}

Error
AmTxManager::closeVolume(fds_volid_t vol_id, fds_int64_t token) {
    return amCache->closeVolume(vol_id, token);
}

void
AmTxManager::statVolume(AmRequest *amReq) {
    amReq->proc_cb = [this, amReq] (Error const& error) mutable -> void {
        if (error.ok()) {
            StatVolumeCallback::ptr cb =
                    SHARED_DYN_CAST(StatVolumeCallback, amReq->cb);
            auto volReq = static_cast<StatVolumeReq *>(amReq);
            cb->current_usage_bytes = volReq->size;
            cb->blob_count = volReq->blob_count;
        }
        processor_cb(amReq, error);
    };
    return amCache->statVolume(amReq);
}

void
AmTxManager::setVolumeMetadata(AmRequest *amReq) {
    amReq->proc_cb = [this, amReq] (Error const& error) mutable -> void {
        processor_cb(amReq, error);
    };
    return amCache->setVolumeMetadata(amReq);
}

void
AmTxManager::getVolumeMetadata(AmRequest *amReq) {
    amReq->proc_cb = [this, amReq] (Error const& error) mutable -> void {
        processor_cb(amReq, error);
    };
    return amCache->getVolumeMetadata(amReq);
}

void
AmTxManager::volumeContents(AmRequest *amReq) {
    amReq->proc_cb = [this, amReq] (Error const& error) mutable -> void {
        processor_cb(amReq, error);
    };
    return amCache->volumeContents(amReq);
}

void
AmTxManager::startBlobTx(AmRequest *amReq) {
    auto blobReq = static_cast<StartBlobTxReq*>(amReq);

    // Callback from dispatch
    amReq->proc_cb = [this, blobReq] (Error const& error) mutable -> void {
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
        processor_cb(blobReq, err);
    };

    // Generate a random transaction ID to use
    blobReq->tx_desc = boost::make_shared<BlobTxId>(randNumGen->genNumSafe());

    return amCache->startBlobTx(amReq);
}

void
AmTxManager::commitBlobTx(AmRequest *amReq) {
    auto blobReq = static_cast<CommitBlobTxReq*>(amReq);

    auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        return processor_cb(amReq, err);
    }

    blobReq->proc_cb = [this, blobReq] (Error const& error) mutable -> void {
        // Push the committed update to the cache and remove from manager
        if (ERR_OK == error) {
            updateStagedBlobDesc(*(blobReq->tx_desc), blobReq->final_meta_data);
            commitTx(*(blobReq->tx_desc), blobReq->final_blob_size);
        } else {
            LOGERROR << "Transaction failed to commit: " << error;
        }
        processor_cb(blobReq, error);
    };
    return amCache->commitBlobTx(amReq);
}

void
AmTxManager::abortBlobTx(AmRequest *amReq) {
    auto blobReq = static_cast<AbortBlobTxReq*>(amReq);
    auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        return processor_cb(amReq, err);
    }
    blobReq->proc_cb = [this, blobReq] (Error const& error) mutable -> void {
        if (ERR_OK != abortTx(*(blobReq->tx_desc))) {
            LOGWARN << "Transaction unknown";
        }
        processor_cb(blobReq, error);
    };

    return amCache->abortBlobTx(amReq);
}

void
AmTxManager::statBlob(AmRequest *amReq) {
    return amCache->statBlob(amReq);
}

void
AmTxManager::setBlobMetadata(AmRequest *amReq) {
    SetBlobMetaDataReq *blobReq = static_cast<SetBlobMetaDataReq *>(amReq);

    // Assert that we have a tx and dmt_version for the message
    auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        return processor_cb(amReq, err);
    }
    amReq->proc_cb = [this, amReq] (Error const& error) mutable -> void {
        processor_cb(amReq, error);
    };

    return amCache->setBlobMetadata(amReq);
}

void
AmTxManager::deleteBlob(AmRequest *amReq) {
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq *>(amReq);
    LOGDEBUG    << " volume:" << amReq->io_vol_id
                << " blob:" << amReq->getBlobName()
                << " txn:" << blobReq->tx_desc;

    // Update the tx manager with the delete op
    auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        return processor_cb(amReq, err);
    }
    updateTxOpType(*(blobReq->tx_desc), amReq->io_type);

    amReq->proc_cb = [this, amReq] (Error const& error) mutable -> void {
        processor_cb(amReq, error);
    };
    return amCache->deleteBlob(amReq);
}

void
AmTxManager::renameBlob(AmRequest *amReq) {
    auto blobReq = static_cast<RenameBlobReq*>(amReq);
    LOGDEBUG << "Renaming blob: " << blobReq->getBlobName()
             << " to: " << blobReq->new_blob_name;

    blobReq->tx_desc.reset(new BlobTxId(randNumGen->genNumSafe()));
    blobReq->dest_tx_desc.reset(new BlobTxId(randNumGen->genNumSafe()));

    return amCache->renameBlob(amReq);
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
        return processor_cb(amReq, ERR_INVALID);
    }

    return amCache->getBlob(amReq);
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

    amReq->proc_cb = [this, blobReq] (Error const& error) mutable -> void {
        putBlobCb(blobReq, error);
    };

    if (amReq->io_type == FDS_PUT_BLOB_ONCE) {
        blobReq->setTxId(randNumGen->genNumSafe());
        /**
         * FEATURE TOGGLE: Update object store before making catalog update.
         * This models the transaction update since the commit comes last to DM.
         * Will more thank likely cause an increase in latency in the write path
         * Wed 19 Aug 2015 10:56:46 AM MDT
         */
        if (safe_atomic_write) {
            // Make data stable on object store prior to updating the catalog, this
            // means sending them in series rather than in parallel.
            amReq->proc_cb = [this, blobReq] (Error const& error) mutable -> void {
                putBlobOnceCb(blobReq, error);
            };

            blobReq->setResponseCount(1);
        } else {
            amCache->putBlobOnce(amReq);
        }
    } else {
        // Verify we have a dmt version (and transaction) for this update
        auto err = getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
        if (!err.ok()) {
            return processor_cb(amReq, err);
        }
        amCache->putBlob(amReq);
    }

    // Either dispatch the put blob request or, if there's no data, just call
    // our callback handler now (NO-OP).
    return amReq->data_len > 0 ? amCache->putObject(amReq) :
                                 blobReq->notifyResponse(ERR_OK);
}

void
AmTxManager::putBlobCb(AmRequest *amReq, Error const& error) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);

    if (error.ok()) {
        auto tx_desc = blobReq->tx_desc;
        // Add the Tx to the manager if this an updateOnce
        if (amReq->io_type == FDS_PUT_BLOB_ONCE) {
            fds_verify(addTx(amReq->io_vol_id,
                             *tx_desc,
                             blobReq->dmt_version,
                             amReq->getBlobName()) == ERR_OK);
            // Stage the transaction metadata changes
            fds_verify(updateStagedBlobDesc(*tx_desc, blobReq->final_meta_data));
        }

        // Update the transaction manager with the stage offset update
        if (ERR_OK != updateStagedBlobOffset(*tx_desc,
                                                    amReq->getBlobName(),
                                                    amReq->blob_offset,
                                                    blobReq->obj_id)) {
            // An abort or commit already caused the tx
            // to be cleaned up. Short-circuit
            GLOGNOTIFY << "Response no longer has active transaction: " << tx_desc->getValue();
            return processor_cb(amReq, ERR_NOT_FOUND);
        }

        // Update the transaction manager with the staged object data
        if (amReq->data_len > 0) {
            updateStagedBlobObject(*tx_desc, blobReq->obj_id, blobReq->dataPtr);
        }

        if (amReq->io_type == FDS_PUT_BLOB_ONCE) {
            commitTx(*tx_desc, blobReq->final_blob_size);
        }
    }

    processor_cb(amReq, error);
}

void
AmTxManager::putBlobOnceCb(AmRequest *amReq, Error const& error) {
    if (!error.ok()) {
        // Skip the volume update, we couldn't even store the data
        return putBlobCb(amReq, error);
    }
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    // Sending the update in a single request. Create transaction ID to
    // use for the single request
    amReq->proc_cb = [this, blobReq] (Error const& error) mutable -> void {
        putBlobCb(blobReq, error);
    };
    blobReq->setResponseCount(1);
    amCache->putBlobOnce(amReq);
}

bool
AmTxManager::getNoNetwork() const
{ return amCache->getNoNetwork(); }

Error
AmTxManager::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
    return amCache->updateDlt(dlt_type, dlt_data, cb);
}

Error
AmTxManager::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
    return amCache->updateDmt(dmt_type, dmt_data, cb);
}

Error
AmTxManager::getDMT() {
    return amCache->getDMT();
}

Error
AmTxManager::getDLT() {
    return amCache->getDLT();
}

}  // namespace fds

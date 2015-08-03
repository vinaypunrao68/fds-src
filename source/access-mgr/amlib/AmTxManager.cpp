/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <map>
#include <string>
#include <fds_process.h>
#include "AmTxManager.h"
#include "AmCache.h"
#include "AmTxDescriptor.h"
#include "requests/GetBlobReq.h"
#include "requests/GetObjectReq.h"

namespace fds {

static constexpr fds_uint32_t Ki { 1024 };
static constexpr fds_uint32_t Mi { 1024 * Ki };

AmTxManager::AmTxManager()
    : amCache(nullptr)
{
}

AmTxManager::~AmTxManager() = default;

void AmTxManager::init(processor_cb_type cb) {
    // This funtion is used to enqueue requests to AmProcessor
    processor_enqueue = cb;
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    maxStagedEntries = conf.get<fds_uint32_t>("cache.tx_max_staged_entries");
    // This is in terms of MiB
    maxPerVolumeCacheSize = Mi * conf.get<fds_uint32_t>("cache.max_volume_data");

    amCache.reset(new AmCache());
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
AmTxManager::registerVolume(const VolumeDesc& volDesc, bool const can_cache_meta)
{
    // The cache size is controlled in terms of MiB, but the LRU
    // knows only terms in # of elements. Do this conversion.
    auto num_cached_objs = (0 < volDesc.maxObjSizeInBytes) ?
        (maxPerVolumeCacheSize / volDesc.maxObjSizeInBytes) : 0;

    // A duplicate is ok, renewal's of a token cause this
    auto err = amCache->registerVolume(volDesc.volUUID, num_cached_objs, can_cache_meta);
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

void
AmTxManager::invalidateMetaCache(const VolumeDesc& volDesc)
{ amCache->invalidateMetaCache(volDesc.volUUID); }

Error
AmTxManager::removeVolume(const VolumeDesc& volDesc)
{ return amCache->removeVolume(volDesc.volUUID); }

Error
AmTxManager::commitTx(const BlobTxId &txId, fds_uint64_t const blobSize)
{
    if (auto descriptor = pop_descriptor(txId)) {
        return amCache->putTxDescriptor(descriptor, blobSize);
    }
    return ERR_NOT_FOUND;
}

BlobDescriptor::ptr
AmTxManager::getBlobDescriptor(fds_volid_t volId, const std::string &blobName, Error &error)
{ return amCache->getBlobDescriptor(volId, blobName, error); }

Error
AmTxManager::getBlobOffsetObjects(fds_volid_t volId, const std::string &blobName, fds_uint64_t const obj_offset, fds_uint64_t const obj_offset_end, size_t const obj_size, std::vector<ObjectID::ptr>& obj_ids)
{ return amCache->getBlobOffsetObjects(volId, blobName, obj_offset, obj_offset_end, obj_size, obj_ids); }

void
AmTxManager::getObjects(GetBlobReq* blobReq) {
    LOGDEBUG << "checking cache for: " << blobReq->object_ids.size() << " objects";
    GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, blobReq->cb);
    cb->return_buffers->assign(blobReq->object_ids.size(), nullptr);
    cb->return_buffers->shrink_to_fit();

    size_t hit_cnt, miss_cnt;
    std::tie(hit_cnt, miss_cnt) = amCache->getObjects(blobReq->io_vol_id,
                                                      blobReq->object_ids,
                                                      *cb->return_buffers);
    if (0 == miss_cnt) {
        // Data was found in cache, done
        LOGTRACE << "Data found in cache!";
        blobReq->proc_cb(ERR_OK);
        return;
    }

    // We did not find all the data, so create some GetObjectReqs and defer to
    // SM for the rest by iterating and checking if the buffer is valid, if
    // not use the object to create a new GetObjectReq
    LOGDEBUG << "Found: " << hit_cnt << "/" << (hit_cnt + miss_cnt) << " objects";
    blobReq->setResponseCount(miss_cnt);
    auto obj_it = blobReq->object_ids.cbegin();
    auto buf_it = cb->return_buffers->begin();
    for (auto end = cb->return_buffers->cend(); end != buf_it; ++obj_it, ++buf_it) {
        if (!*buf_it) {
            LOGDEBUG << "Instantiating GetObject for object: " << **obj_it;
            getObject(blobReq, *obj_it, *buf_it);
        }
    }
}

void
AmTxManager::getObject(GetBlobReq* blobReq,
                       ObjectID::ptr const& obj_id,
                       boost::shared_ptr<std::string>& buf) {
    auto objReq = new GetObjectReq(blobReq, buf, obj_id);
    objReq->proc_cb = [this, objId = *obj_id] (Error const& error) {
                        this->getObjectCb(objId, error);
                      };

    std::lock_guard<std::mutex> g(obj_get_lock);
    auto q = obj_get_queue.find(*obj_id);
    if (obj_get_queue.end() == q) {
        processor_enqueue(objReq);
        auto new_queue = new std::deque<GetObjectReq*>();
        obj_get_queue[*obj_id].reset(new_queue);
        new_queue->push_back(objReq);
    } else {
        q->second->push_back(objReq);
    }
}

void
AmTxManager::getObjectCb(ObjectID const obj_id, Error const& error) {
    std::unique_ptr<std::deque<GetObjectReq*>> queue;
    {
        // Find the waiting get requeust queue
        std::lock_guard<std::mutex> g(obj_get_lock);
        auto q = obj_get_queue.find(obj_id);
        fds_assert(obj_get_queue.end() != q);
        queue.swap(q->second);
        obj_get_queue.erase(q);
    }

    // For each request waiting on this object, respond to it and set the buffer
    // data member from the first request (the one that was actually dispatched)
    // and populate the volume's cache
    auto buf = queue->front()->obj_data;
    for (auto& objReq : *queue) {
        if (error.ok()) {
            objReq->obj_data = buf;
            amCache->putObject(objReq->io_vol_id, *objReq->obj_id, objReq->obj_data);
        }
        objReq->blobReq->notifyResponse(error);
        delete objReq;
    }
}

Error
AmTxManager::putOffsets(fds_volid_t const vol_id,
                        std::string const& blob_name,
                        fds_uint64_t const blob_offset,
                        fds_uint32_t const object_size,
                        std::vector<boost::shared_ptr<ObjectID>> const& object_ids) {
    auto iOff = blob_offset;
    for (auto const& obj_id : object_ids) {
        auto blob_pair = BlobOffsetPair(blob_name, iOff);
        auto err = amCache->putOffset(vol_id, blob_pair, obj_id);
        if (!err.ok())
            { return err; }
        iOff += object_size;
    }
    return ERR_OK;
}

Error
AmTxManager::putBlobDescriptor(fds_volid_t const volId, std::string const& blobName, boost::shared_ptr<BlobDescriptor> const blobDesc)
{ return amCache->putBlobDescriptor(volId, blobName, blobDesc); }

Error
AmTxManager::removeBlob(fds_volid_t volId, const std::string &blobName)
{ return amCache->removeBlob(volId, blobName); }

}  // namespace fds

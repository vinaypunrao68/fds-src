/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <map>
#include <string>
#include <fds_process.h>
#include "AmTxManager.h"
#include "AmCache.h"
#include "AmTxDescriptor.h"
#include "AmVolumeTable.h"

namespace fds {

static constexpr fds_uint32_t Ki { 1024 };
static constexpr fds_uint32_t Mi { 1024 * Ki };

AmTxManager::AmTxManager()
    : amCache(nullptr),
      volTable(nullptr)
{
}

AmTxManager::~AmTxManager() = default;

void AmTxManager::init(processor_callback_type&& cb) {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    maxStagedEntries = conf.get<fds_uint32_t>("cache.tx_max_staged_entries");
    // This is in terms of MiB
    maxPerVolumeCacheSize = Mi * conf.get<fds_uint32_t>("cache.max_volume_data");
    qos_threads = conf.get<int>("qos_threads");

    amCache.reset(new AmCache());
    volTable.reset(new AmVolumeTable(qos_threads, GetLog()));

    auto closure = [cb](AmRequest* amReq) mutable -> void { cb(amReq); };
    volTable->registerCallback(std::move(closure));
}

Error
AmTxManager::enqueueRequest(AmRequest* amReq) {
    return volTable->enqueueRequest(amReq);
}

Error
AmTxManager::markIODone(AmRequest* amReq) {
    return volTable->markIODone(amReq);
}

bool
AmTxManager::drained() {
    return volTable->drained();
}

Error
AmTxManager::updateQoS(long int const* rate,
                       float const* throttle) {
    return volTable->updateQoS(rate, throttle);
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
    SCOPEDWRITE(txMapLock);
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
    SCOPEDWRITE(txMapLock);
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
    SCOPEDWRITE(txMapLock);
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
AmTxManager::registerVolume(const VolumeDesc& volDesc,
                           boost::shared_ptr<AmVolumeAccessToken> access_token)
{
    // The cache size is controlled in terms of MiB, but the LRU
    // knows only terms in # of elements. Do this conversion.
    auto num_cached_objs = (maxPerVolumeCacheSize / volDesc.maxObjSizeInBytes);

    // A duplicate is ok, we're probably updating the access_token
    auto err = amCache->registerVolume(volDesc.volUUID, num_cached_objs);
    if ((ERR_OK == err) || (ERR_DUPLICATE == err)) {
        LOGDEBUG << "Created caches for volume: " << std::hex << volDesc.volUUID;
        err = volTable->registerVolume(volDesc, access_token);
        if (ERR_OK != err) {
            amCache->removeVolume(volDesc.volUUID);
        }
    }

    if (!err.ok()) {
        LOGERROR << "Failed to register volume: " << err;
    }
    return err;
}

Error
AmTxManager::modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc)
{ return volTable->modifyVolumePolicy(vol_uuid, vdesc); }

Error
AmTxManager::removeVolume(const VolumeDesc& volDesc)
{
    auto err = volTable->removeVolume(volDesc);
    if (ERR_OK == err)
      { amCache->removeVolume(volDesc.volUUID); }
    return err;
}

Error
AmTxManager::commitTx(const BlobTxId &txId, fds_uint64_t const blobSize)
{
    if (auto descriptor = pop_descriptor(txId)) {
        return amCache->putTxDescriptor(descriptor, blobSize);
    }
    return ERR_NOT_FOUND;
}

AmVolumeTable::volume_ptr_type
AmTxManager::getVolume(fds_volid_t vol_uuid) const
{ return volTable->getVolume(vol_uuid); }

void
AmTxManager::getVolumeTokens(std::deque<std::pair<fds_volid_t, fds_int64_t>>& tokens) const
{ return volTable->getVolumeTokens(tokens); }

BlobDescriptor::ptr
AmTxManager::getBlobDescriptor(fds_volid_t volId, const std::string &blobName, Error &error)
{ return amCache->getBlobDescriptor(volId, blobName, error); }

ObjectID::ptr
AmTxManager::getBlobOffsetObject(fds_volid_t volId, const std::string &blobName, fds_uint64_t blobOffset, Error &error)
{ return amCache->getBlobOffsetObject(volId, blobName, blobOffset, error); }

Error
AmTxManager::putObject(fds_volid_t const volId, ObjectID const& objId, boost::shared_ptr<std::string> const obj)
{ return amCache->putObject(volId, objId, obj); }

boost::shared_ptr<std::string>
AmTxManager::getBlobObject(fds_volid_t volId, const ObjectID &objectId, Error &error)
{ return amCache->getBlobObject(volId, objectId, error); }

Error
AmTxManager::putOffset(fds_volid_t const volId, BlobOffsetPair const& blobOff, boost::shared_ptr<ObjectID> const objId)
{ return amCache->putOffset(volId, blobOff, objId); }

Error
AmTxManager::putBlobDescriptor(fds_volid_t const volId, std::string const& blobName, boost::shared_ptr<BlobDescriptor> const blobDesc)
{ return amCache->putBlobDescriptor(volId, blobName, blobDesc); }

}  // namespace fds

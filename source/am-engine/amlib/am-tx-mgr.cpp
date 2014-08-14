/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <map>
#include <string>
#include <am-tx-mgr.h>

namespace fds {

AmTxDescriptor::AmTxDescriptor(fds_volid_t volUuid,
                               const BlobTxId &id,
                               fds_uint64_t dmtVer,
                               const std::string &name)
        : volId(volUuid),
          txId(id),
          originDmtVersion(dmtVer),
          blobName(name),
          stagedBlobDesc(new BlobDescriptor()) {
    stagedBlobDesc->setVolId(volId);
    stagedBlobDesc->setBlobName(blobName);
    stagedBlobDesc->setBlobSize(0);
    // TODO(Andrew): We're leaving the blob version unset
    // We'll need to revist when we do versioning
}

AmTxDescriptor::~AmTxDescriptor() {
}

AmTxManager::AmTxManager(const std::string &modName)
        : Module(modName.c_str()) {
}

AmTxManager::~AmTxManager() {
}

/**
 * Module initialization
 */
int
AmTxManager::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
AmTxManager::mod_startup() {
}

/**
 * Module shutdown
 */
void
AmTxManager::mod_shutdown() {
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
    txMap[txId] = AmTxDescriptor::ptr(new AmTxDescriptor(volId, txId, dmtVer, name));

    return ERR_OK;
}

Error
AmTxManager::removeTx(const BlobTxId &txId) {
    SCOPEDWRITE(txMapLock);
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    fds_verify(txId == txMapIt->second->txId);
    txMap.erase(txMapIt);
    return ERR_OK;
}

Error
AmTxManager::getTxDescriptor(const BlobTxId &txId, AmTxDescriptor::ptr &desc) {
    SCOPEDWRITE(txMapLock);
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    fds_verify(txId == txMapIt->second->txId);
    desc = txMapIt->second;
    return ERR_OK;
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
AmTxManager::updateStagedBlobDesc(const BlobTxId &txId,
                                  boost::shared_ptr<
                                  fpi::FDSP_MetaDataList> metaDataList) {
    SCOPEDWRITE(txMapLock);
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    fds_verify(txId == txMapIt->second->txId);
    // Verify that we're not overwriting previously staged metadata
    fds_verify(txMapIt->second->stagedBlobDesc->kvMetaBegin() ==
               txMapIt->second->stagedBlobDesc->kvMetaEnd());

    for (auto const & meta : *metaDataList) {
        txMapIt->second->stagedBlobDesc->addKvMeta(meta.key, meta.value);
    }
    return ERR_OK;
}

Error
AmTxManager::updateStagedBlobDesc(const BlobTxId &txId,
                                  boost::shared_ptr<
                                  std::map<std::string, std::string>> &metadata) {
    SCOPEDWRITE(txMapLock);
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    fds_verify(txId == txMapIt->second->txId);
    // Verify that we're not overwriting previously staged metadata
    fds_verify(txMapIt->second->stagedBlobDesc->kvMetaBegin() ==
               txMapIt->second->stagedBlobDesc->kvMetaEnd());

    for (auto const & meta : *metadata) {
        txMapIt->second->stagedBlobDesc->addKvMeta(meta.first, meta.second);
    }
    return ERR_OK;
}

Error
AmTxManager::updateStagedBlobDesc(const BlobTxId &txId,
                                  fds_uint64_t len) {
    SCOPEDWRITE(txMapLock);
    TxMap::iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    fds_verify(txId == txMapIt->second->txId);
    txMapIt->second->stagedBlobDesc->updateBlobSize(len);
    return ERR_OK;
}

}  // namespace fds

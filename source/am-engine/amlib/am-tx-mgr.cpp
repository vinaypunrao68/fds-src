/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <am-tx-mgr.h>

namespace fds {

AmTxDescriptor::AmTxDescriptor(const BlobTxId &id,
                               fds_uint64_t dmtVer,
                               const std::string &name)
        :txId(id), originDmtVersion(dmtVer), blobName(name) {
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
AmTxManager::addTx(const BlobTxId &txId, fds_uint64_t dmtVer, const std::string &name) {
    SCOPEDWRITE(txMapLock);
    if (txMap.count(txId) > 0) {
        return ERR_DUPLICATE_UUID;
    }
    txMap[txId] = AmTxDescriptor::ptr(new AmTxDescriptor(txId, dmtVer, name));

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
AmTxManager::getTxDmtVersion(const BlobTxId &txId, fds_uint64_t *dmtVer) const {
    SCOPEDREAD(txMapLock);
    TxMap::const_iterator txMapIt = txMap.find(txId);
    if (txMapIt == txMap.end()) {
        return ERR_NOT_FOUND;
    }
    *dmtVer = txMapIt->second->originDmtVersion;
    return ERR_OK;
}

}  // namespace fds

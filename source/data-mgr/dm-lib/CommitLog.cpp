/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <limits>

#include <CommitLog.h>

namespace {

std::atomic<fds_uint64_t> epoch(0);

} /* anonymous namespace */

namespace fds {

DmCommitLog::DmCommitLog(const std::string &modName, PersistenceType persist /* = IN_FILE */)
        : Module(modName.c_str()), persist_(persist) {}

DmCommitLog::~DmCommitLog() {}

/**
 * Module initialization
 */
int
DmCommitLog::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
DmCommitLog::mod_startup() {
}

/**
 * Module shutdown
 */
void
DmCommitLog::mod_shutdown() {
}

/*
 * operations
 */
// start transaction
Error DmCommitLog::startTx(BlobTxId::const_ptr & txDesc, const std::string & blobName) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Starting blob transaction (" << txId << ") for (" << blobName << ")";

    TxMap::const_iterator iter = txMap_.find(txId);
    if (txMap_.end() != iter) {
        LOGWARN << "Blob transaction already exists";
        return ERR_DM_TX_EXISTS;
    }

    txMap_[txId].txDesc = txDesc;
    txMap_[txId].blobName = blobName;

    fds_uint64_t id = 0;
    Error rc = ERR_OK;  // TODO(umesh): cmtLogger_->startTx(txDesc, blobName, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    txMap_[txId].entries.push_back(id);
    txMap_[txId].started = true;

    return rc;
}

// update blob data (T can be BlobObjList or BlobMetaDesc)
template<typename T>
Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc, boost::shared_ptr<const T> & blobData) {
    fds_assert(txDesc);
    fds_assert(blobData);
    fds_assert(cmtLogger_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Update blob for transaction (" << txId << ")";

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    fds_uint64_t id = 0;
    rc = ERR_OK;  // TODO(umesh): cmtLogger_->updateTx(txDesc, blobData, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    // TODO(umesh): upsert blobData

    return ERR_OK;
}

// commit transaction
Error DmCommitLog::commitTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Commiting blob transaction " << *txDesc;
    return ERR_OK;
}

// rollback transaction
Error DmCommitLog::rollbackTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Rollback blob transaction " << *txDesc;
    return ERR_OK;
}

// purge transaction
Error DmCommitLog::purgeTx(BlobTxId::const_ptr  & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Purge blob transaction " << *txDesc;
    return ERR_OK;
}

// get transaction
Error DmCommitLog::getTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Get blob transaction " << txDesc;
    return ERR_OK;
}

Error DmCommitLog::validateSubsequentTx(const BlobTxId & txId) {
    TxMap::iterator iter = txMap_.find(txId);
    if (txMap_.end() == iter) {
        LOGERROR << "Blob transaction not started";
        return ERR_DM_TX_NOT_STARTED;
    }
    fds_assert(txId == *(iter->second.txDesc));

    if (iter->second.rolledback) {
        LOGERROR << "Blob transaction already rolled back";
        return ERR_DM_TX_ROLLEDBACK;
    } else if (iter->second.commited) {
        LOGERROR << "Blob transaction already commited";
        return ERR_DM_TX_COMMITED;
    }

    return ERR_OK;
}

DmCommitLogEntry::DmCommitLogEntry(CommitLogEntryType type_, const BlobTxId & txDesc)
        : stale(0), type(type_), id(0), txId(txDesc.getValue()), prev(0) {}

DmCommitLogStartEntry::DmCommitLogStartEntry(const BlobTxId & txDesc, const std::string & blobName)
        : DmCommitLogEntry(TX_START, txDesc) {
    boost::scoped_ptr<serialize::Serializer> s(serialize::getMemSerializer());
    s->writeString(blobName);
    const std::string & str = s->getBufferAsString();
    uint32_t b = str.size();
    fds_assert(b < DETAILS_BUFFER_SIZE);

    memcpy(details, str.c_str(), b);
    details[b] = 0;
}

std::string DmCommitLogStartEntry::blobName() const {
    fds_assert(TX_START == type);
    boost::scoped_ptr<serialize::Deserializer> d(serialize::getMemDeserializer(details));
    std::string retStr;
    d->readString(retStr);
    return retStr;
}

DmCommitLogUpdateObjListEntry::DmCommitLogUpdateObjListEntry(const BlobTxId & txDesc,
        const BlobObjList::ptr & blobObjList) : DmCommitLogEntry(TX_UPDATE_OBJLIST, txDesc) {
    boost::scoped_ptr<serialize::Serializer> s(serialize::getMemSerializer(DETAILS_BUFFER_SIZE));
    blobObjList->write(s.get());
    const std::string & str = s->getBufferAsString();
    uint32_t b = str.size();
    fds_assert(b < DETAILS_BUFFER_SIZE);

    memcpy(details, str.c_str(), b);
    details[b] = 0;
}

DmCommitLogUpdateObjMetaEntry::DmCommitLogUpdateObjMetaEntry(const BlobTxId & txDesc,
        const BlobMetaDesc::ptr & meta) : DmCommitLogEntry(TX_UPDATE_OBJMETA, txDesc) {
    boost::scoped_ptr<serialize::Serializer> s(
            serialize::getMemSerializer(DETAILS_BUFFER_SIZE));
    meta->write(s.get());
    const std::string & str = s->getBufferAsString();
    uint32_t b = str.size();
    fds_assert(b < DETAILS_BUFFER_SIZE);

    memcpy(details, str.c_str(), b);
    details[b] = 0;
}

}  /* namespace fds */

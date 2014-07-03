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
    LOGDEBUG << "Starting blob transaction " << *txDesc << " for " << blobName;
    return ERR_OK;
}

// update blob obj list
Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc, BlobObjList::const_ptr & blobObjList) {
    fds_assert(txDesc);
    LOGDEBUG << "Update blob for transaction " << *txDesc;
    return ERR_OK;
}

// update blob meta data
Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc, BlobMetaDesc::const_ptr & blobMetaDesc) {
    fds_assert(txDesc);
    LOGDEBUG << "Update blob meta data for transaction " << *txDesc;
    return ERR_OK;
}

// commit transaction
Error DmCommitLog::commitTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    LOGDEBUG << "Commiting blob transaction " << *txDesc;
    return ERR_OK;
}

// rollback transaction
Error DmCommitLog::rollbackTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    LOGDEBUG << "Rollback blob transaction " << *txDesc;
    return ERR_OK;
}

// purge transaction
Error DmCommitLog::purgeTx(BlobTxId::const_ptr  & txDesc) {
    fds_assert(txDesc);
    LOGDEBUG << "Purge blob transaction " << *txDesc;
    return ERR_OK;
}

DmCommitLogEntry::DmCommitLogEntry(CommitLogEntryType type_, const BlobTxId & txDesc)
        : stale(0), type(type_), id(0), txId(txDesc.getValue()), prev(0), next(0) {}

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
    blobObjList->write(s);
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
    meta->write(s);
    const std::string & str = s->getBufferAsString();
    uint32_t b = str.size();
    fds_assert(b < DETAILS_BUFFER_SIZE);

    memcpy(details, str.c_str(), b);
    details[b] = 0;
}

}  /* namespace fds */

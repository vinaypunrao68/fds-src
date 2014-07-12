/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>

#include <system_error>
#include <string>
#include <limits>
#include <deque>

#include <dm-tvc/CommitLog.h>

#define FDS_PAGE_START_ADDR(x) (reinterpret_cast<uintptr_t>(x) & ~(FDS_PAGE_SIZE - 1))

#define FDS_PAGE_OFFSET(x) (reinterpret_cast<uintptr_t>(x) & (FDS_PAGE_SIZE - 1))

namespace {

std::atomic<fds_uint64_t> clEpoch(0);

const fds_uint64_t FDS_PAGE_SIZE = sysconf(_SC_PAGESIZE);

} /* anonymous namespace */

namespace fds {

template Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc,
                        boost::shared_ptr<const MetaDataList> & blobData);
template Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc,
                        boost::shared_ptr<const BlobObjList> & blobData);

DmCommitLog::DmCommitLog(const std::string &modName, const std::string & filename,
        fds_uint32_t filesize /* = DEFAULT_COMMIT_LOG_FILE_SIZE */,
        PersistenceType persist /* = IN_MEMORY */) : Module(modName.c_str()), filename_(filename),
        filesize_(filesize), persist_(persist), started_(false) {
    if (IN_FILE == persist_) {
        cmtLogger_.reset(new FileCommitLogger(filename_, filesize_));
    } else if (IN_MEMORY == persist_) {
        cmtLogger_.reset(new MemoryCommitLogger());
    } else {
        // TODO(umesh): instantiate leveldb based cmtLogger_
    }

    // populate txMap_
    for (const DmCommitLogEntry * entry = cmtLogger_->getFirst(); 0 != entry;
            entry = cmtLogger_->getNext(entry)) {
        BlobTxId txId(entry->txId);

        TxMap::iterator iter = txMap_.find(txId);
        fds_verify((TX_START == entry->type && txMap_.end() == iter) ||
                (TX_START != entry->type && txMap_.end() != iter));

        if (TX_START == entry->type) {
            txMap_[txId].reset(new CommitLogTx());
            txMap_[txId]->txDesc.reset(new BlobTxId(entry->txId));
            txMap_[txId]->blobName =
                    reinterpret_cast<const DmCommitLogStartEntry *>(entry)->blobName();
            txMap_[txId]->entries.push_back(entry->id);
            txMap_[txId]->started = true;
        } else {
            fds_verify(iter->second->started);
            fds_verify(!iter->second->commited);
            fds_verify(!iter->second->rolledback);

            iter->second->entries.push_back(entry->id);

            switch (entry->type) {
                case TX_COMMIT:
                    iter->second->commited = true;
                    break;
                case TX_ROLLBACK:
                    iter->second->rolledback = true;
                    break;
                case TX_UPDATE_OBJLIST: {
                    boost::shared_ptr<const BlobObjList> dataPtr =
                            reinterpret_cast<const DmCommitLogUpdateObjListEntry *>\
                            (entry)->blobObjList();
                    upsertBlobData(*(txMap_[txId]), dataPtr);
                    break;
                }
                case TX_UPDATE_OBJMETA: {
                    boost::shared_ptr<const MetaDataList> dataPtr =
                            reinterpret_cast<const DmCommitLogUpdateObjMetaEntry *>\
                            (entry)->metaDataList();
                    upsertBlobData(*(txMap_[txId]), dataPtr);
                    break;
                }
                default:
                    break;
            }
        }
    }
}

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
    Module::mod_startup();
    started_ = true;
}

/**
 * Module shutdown
 */
void
DmCommitLog::mod_shutdown() {
    Module::mod_shutdown();
    started_ = false;
}

/*
 * operations
 */
// start transaction
Error DmCommitLog::startTx(BlobTxId::const_ptr & txDesc, const std::string & blobName) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Starting blob transaction (" << txId << ") for (" << blobName << ")";

    {
        SCOPEDREAD(lockTxMap_);

        TxMap::const_iterator iter = txMap_.find(txId);
        if (txMap_.end() != iter) {
            LOGWARN << "Blob transaction already exists";
            return ERR_DM_TX_EXISTS;
        }
    }

    fds_uint64_t id = 0;
    Error rc = cmtLogger_->startTx(txDesc, blobName, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId].reset(new CommitLogTx());
    txMap_[txId]->txDesc = txDesc;
    txMap_[txId]->blobName = blobName;
    txMap_[txId]->entries.push_back(id);
    txMap_[txId]->started = true;

    return rc;
}

// update blob data (T can be BlobObjList or MetaDataList)
template<typename T>
Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc, boost::shared_ptr<const T> & blobData) {
    fds_assert(txDesc);
    fds_assert(blobData);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Update blob for transaction (" << txId << ")";

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    fds_uint64_t id = 0;
    rc = cmtLogger_->updateTx(txDesc, blobData, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId]->entries.push_back(id);

    upsertBlobData(*txMap_[txId], blobData);

    return ERR_OK;
}

// commit transaction
CommitLogTx::const_ptr DmCommitLog::commitTx(BlobTxId::const_ptr & txDesc, Error & status) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Commiting blob transaction " << txId;

    status = validateSubsequentTx(txId);
    if (!status.ok()) {
        return 0;
    }

    fds_uint64_t id = 0;
    status = cmtLogger_->commitTx(txDesc, id);
    if (!status.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << status << ")";
        return 0;
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId]->entries.push_back(id);
    txMap_[txId]->commited = true;

    return txMap_[txId];
}

// rollback transaction
Error DmCommitLog::rollbackTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Rollback blob transaction " << txId;

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    fds_uint64_t id = 0;
    rc = cmtLogger_->rollbackTx(txDesc, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId]->entries.push_back(id);
    txMap_[txId]->rolledback = true;

    // TODO(umesh): possibly erase from map and notify GC

    return rc;
}

// purge transaction
Error DmCommitLog::purgeTx(BlobTxId::const_ptr  & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Purge blob transaction " << txId;

    {
        SCOPEDREAD(lockTxMap_);

        TxMap::iterator iter = txMap_.find(txId);
        if (txMap_.end() == iter) {
            LOGERROR << "Blob transaction not started";
            return ERR_DM_TX_NOT_STARTED;
        }
        fds_assert(txId == *(iter->second->txDesc));

        if (!iter->second->rolledback && !iter->second->commited) {
            LOGERROR << "Blob transaction active, can not be purged";
            return ERR_DM_TX_ACTIVE;
        }
    }

    fds_uint64_t id = 0;
    Error rc = cmtLogger_->purgeTx(txDesc, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId]->entries.push_back(id);

    // TODO(umesh): erase from map and possibly notify GC

    return ERR_OK;
}

// get transaction
CommitLogTx::const_ptr DmCommitLog::getTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Get blob transaction " << txId;

    SCOPEDREAD(lockTxMap_);

    TxMap::const_iterator iter = txMap_.find(txId);
    if (txMap_.end() != iter) {
        return iter->second;
    }

    return 0;
}

Error DmCommitLog::validateSubsequentTx(const BlobTxId & txId) {
    SCOPEDREAD(lockTxMap_);

    TxMap::iterator iter = txMap_.find(txId);
    if (txMap_.end() == iter) {
        LOGERROR << "Blob transaction not started";
        return ERR_DM_TX_NOT_STARTED;
    }
    fds_assert(txId == *(iter->second->txDesc));

    if (iter->second->rolledback) {
        LOGERROR << "Blob transaction already rolled back";
        return ERR_DM_TX_ROLLEDBACK;
    } else if (iter->second->commited) {
        LOGERROR << "Blob transaction already commited";
        return ERR_DM_TX_COMMITED;
    }

    return ERR_OK;
}

template<typename T>
const std::string DmCommitLogEntry::createPayload(boost::shared_ptr<const T> val) {
    boost::scoped_ptr<serialize::Serializer> s(serialize::getMemSerializer());
    val->write(s.get());
    return s->getBufferAsString();
}

void FileCommitLogger::addEntryToHeader(DmCommitLogEntry * entry) {
    fds_uint32_t offset = entryOffset(entry);
    last()->next = offset;
    header()->last = offset;
    ++(header()->count);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(header())), sizeof(EntriesHeader),
            MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log header";
    }
}

FileCommitLogger::FileCommitLogger(const std::string & filename, fds_uint32_t filesize)
        : filename_(filename), filesize_(filesize), fd_(-1), prot_(PROT_READ | PROT_WRITE),
        flags_(MAP_PRIVATE | MAP_ANONYMOUS), addr_(0), lockLogFile_("commit log file lock") {
    bool create = true;

    if (!filename.empty()) {
        LOGNORMAL << "Writing commit log to file '" << filename << "'";

        flags_ = MAP_SHARED;
        fd_ = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
        if (fd_ < 0) {
            throw std::system_error(errno, std::system_category());
        }

        struct stat st;
        if (fstat(fd_, &st)) {
            throw std::system_error(errno, std::system_category());
        }
        create = (st.st_size == 0);

        if (st.st_size < filesize_) {
            if (ftruncate(fd_, filesize_)) {
                throw std::system_error(errno, std::system_category());
            }
        }
    }

    addr_ = reinterpret_cast<char *>(mmap(NULL, filesize, prot_, flags_, fd_, 0));
    if (MAP_FAILED == addr_) {
        throw std::system_error(errno, std::system_category());
    }

    if (create) {
        header()->count = 0;
        header()->first = entryOffset(reinterpret_cast<DmCommitLogEntry *>(data()));
        header()->last = entryOffset(reinterpret_cast<DmCommitLogEntry *>(data()));
    }

    // TODO(umesh): verify checksum and check file content integrity

    // basic checks
    fds_verify(header()->first < filesize_ && header()->first >= sizeof(EntriesHeader));
    fds_verify(header()->last < filesize_ && header()->last >= sizeof(EntriesHeader));
}

FileCommitLogger::~FileCommitLogger() {
    if (0 != munmap(addr_, filesize_)) {
        throw std::system_error(errno, std::system_category());
    }
}

char * FileCommitLogger::commitLogAlloc(const size_t sz) {
    fds_assert(sz);
    fds_assert(sz < (filesize_ - sizeof(EntriesHeader)));

    if (!header()->count) {
        return data();  // inserting first record
    }

    char * ret = reinterpret_cast<char *>(last()) + last()->length();
    if ((ret + sz) > (addr_ + filesize_)) {
        ret = data();   // reached end of file, rotate
    }

    if (ret <= reinterpret_cast<char *>(first())) {
        if ((ret + sz) > reinterpret_cast<char *>(first())) {
            return 0;    // reached filesize limit
        }
    }

    return ret;
}

Error FileCommitLogger::startTx(BlobTxId::const_ptr & txDesc, const std::string & blobName,
        fds_uint64_t & id) {
    size_t sz = sizeof(DmCommitLogEntry) + blobName.length() + 1;

    FDSGUARD(lockLogFile_);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    id = clEpoch++;

    DmCommitLogStartEntry * entry = new(clp) DmCommitLogStartEntry(txDesc, id, blobName);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::updateTx(BlobTxId::const_ptr & txDesc, BlobObjList::const_ptr blobObjList,
        fds_uint64_t & id) {
    const std::string & str = DmCommitLogEntry::createPayload(blobObjList);

    FDSGUARD(lockLogFile_);

    size_t sz = sizeof(DmCommitLogEntry) + str.length() + 1;
    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    id = clEpoch++;
    DmCommitLogUpdateObjListEntry * entry = new(clp) DmCommitLogUpdateObjListEntry(txDesc, id, str);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::updateTx(BlobTxId::const_ptr & txDesc, MetaDataList::const_ptr metaDataList,
        fds_uint64_t & id) {
    const std::string & str = DmCommitLogEntry::createPayload(metaDataList);
    size_t sz = sizeof(DmCommitLogEntry) + str.length() + 1;

    FDSGUARD(lockLogFile_);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    id = clEpoch++;
    DmCommitLogUpdateObjMetaEntry * entry = new(clp) DmCommitLogUpdateObjMetaEntry(txDesc, id, str);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::commitTx(BlobTxId::const_ptr & txDesc, fds_uint64_t & id) {
    size_t sz = sizeof(DmCommitLogEntry) + 1;

    FDSGUARD(lockLogFile_);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    id = clEpoch++;
    DmCommitLogCommitEntry * entry = new(clp) DmCommitLogCommitEntry(txDesc, id);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::rollbackTx(BlobTxId::const_ptr & txDesc, fds_uint64_t & id) {
    size_t sz = sizeof(DmCommitLogEntry) + 1;

    FDSGUARD(lockLogFile_);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    id = clEpoch++;
    DmCommitLogRollbackEntry * entry = new(clp) DmCommitLogRollbackEntry(txDesc, id);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::purgeTx(BlobTxId::const_ptr & txDesc, fds_uint64_t & id) {
    size_t sz = sizeof(DmCommitLogEntry) + 1;

    FDSGUARD(lockLogFile_);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    id = clEpoch++;
    DmCommitLogPurgeEntry * entry = new(clp) DmCommitLogPurgeEntry(txDesc, id);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

const DmCommitLogEntry * FileCommitLogger::getNextInternal(const DmCommitLogEntry * rhs) {
    fds_assert(rhs);
    fds_assert(header()->count);
    fds_verify(first() <= rhs && last() >= rhs);

    if (!rhs->next) {
        return 0;   // last entry
    }

    return reinterpret_cast<DmCommitLogEntry *>(addr_ + rhs->next);
}

const DmCommitLogEntry * FileCommitLogger::getEntry(const fds_uint64_t id) {
    FDSGUARD(lockLogFile_);
    const DmCommitLogEntry * ret = getFirstInternal();
    for (; 0 != ret; ret = getNextInternal(ret)) {
        if (id == ret->id) {
            break;
        }
    }
    return ret;
}

}  /* namespace fds */

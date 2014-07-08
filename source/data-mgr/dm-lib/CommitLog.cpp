/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#include <system_error>
#include <string>
#include <limits>
#include <deque>

#include <CommitLog.h>

/*
#define FDS_PAGE_START_ADDR(x) (reinterpret_cast<unsigned *>(x) & \
        reinterpret_cast<unsigned *>(~(FDS_PAGE_SIZE - 1)))
*/

namespace {

std::atomic<fds_uint64_t> clEpoch(0);

const unsigned FDS_PAGE_SIZE = sysconf(_SC_PAGESIZE);

} /* anonymous namespace */

namespace fds {

DmCommitLog::DmCommitLog(const std::string &modName, const std::string & filename,
        fds_uint32_t filesize /* = DEFAULT_COMMIT_LOG_FILE_SIZE */,
        PersistenceType persist /* = IN_MEMORY */) : Module(modName.c_str()), filename_(filename),
        filesize_(filesize), persist_(persist) {
    if (IN_FILE == persist_) {
        cmtLogger_.reset(new FileCommitLogger(filename_, filesize_));
    } else if (IN_MEMORY == persist_) {
        cmtLogger_.reset(new MemoryCommitLogger());
    } else {
        // TODO(umesh): instantiate leveldb based cmtLogger_
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
    started_ = true;
}

/**
 * Module shutdown
 */
void
DmCommitLog::mod_shutdown() {
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

    TxMap::const_iterator iter = txMap_.find(txId);
    if (txMap_.end() != iter) {
        LOGWARN << "Blob transaction already exists";
        return ERR_DM_TX_EXISTS;
    }

    fds_uint64_t id = 0;
    Error rc = ERR_OK;  // TODO(umesh): cmtLogger_->startTx(txDesc, blobName, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    txMap_[txId].txDesc = txDesc;
    txMap_[txId].blobName = blobName;
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
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Update blob for transaction (" << txId << ")";

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    fds_uint64_t preId = txMap_[txId].entries.back();

    fds_uint64_t id = 0;
    rc = ERR_OK;  // TODO(umesh): cmtLogger_->updateTx(txDesc, prevId, blobData, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    txMap_[txId].entries.push_back(id);

    // TODO(umesh): upsert blobData

    return ERR_OK;
}

// commit transaction
Error DmCommitLog::commitTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Commiting blob transaction " << *txDesc;

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    fds_uint64_t preId = txMap_[txId].entries.back();

    fds_uint64_t id = 0;
    rc = ERR_OK;  // TODO(umesh): cmtLogger_->commitTx(txDesc, prevId, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }
    txMap_[txId].entries.push_back(id);
    txMap_[txId].commited = true;

    return rc;
}

// rollback transaction
Error DmCommitLog::rollbackTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Rollback blob transaction " << *txDesc;

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    fds_uint64_t preId = txMap_[txId].entries.back();

    fds_uint64_t id = 0;
    rc = ERR_OK;  // TODO(umesh): cmtLogger_->rollbackTx(txDesc, prevId, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }
    txMap_[txId].entries.push_back(id);
    txMap_[txId].rolledback = true;

    // TODO(umesh): possibly erase from map and notify GC

    return rc;
}

// purge transaction
Error DmCommitLog::purgeTx(BlobTxId::const_ptr  & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    LOGDEBUG << "Purge blob transaction " << *txDesc;

    TxMap::iterator iter = txMap_.find(txId);
    if (txMap_.end() == iter) {
        LOGERROR << "Blob transaction not started";
        return ERR_DM_TX_NOT_STARTED;
    }
    fds_assert(txId == *(iter->second.txDesc));

    if (!iter->second.rolledback && !iter->second.commited) {
        LOGERROR << "Blob transaction active, can not be purged";
        return ERR_DM_TX_ACTIVE;
    }

    fds_uint64_t preId = txMap_[txId].entries.back();

    fds_uint64_t id = 0;
    Error rc = ERR_OK;  // TODO(umesh): cmtLogger_->purgeTx(txDesc, prevId, id);
    if (!rc.ok()) {
        LOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    txMap_[txId].entries.push_back(id);

    // TODO(umesh): erase from map and possibly notify GC

    return ERR_OK;
}

// get transaction
Error DmCommitLog::getTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

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

    if (0 != msync(addr_, sizeof(EntriesHeader), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log header";
    }
}

FileCommitLogger::FileCommitLogger(const std::string & filename, fds_uint32_t filesize)
        : filename_(filename), filesize_(filesize), fd_(-1), prot_(PROT_READ | PROT_WRITE),
        flags_(MAP_PRIVATE | MAP_ANONYMOUS), addr_(0) {
    bool create = true;

    if (!filename.empty()) {
        LOGNORMAL << "Writing commit log to file '" << filename << "'";
        // TODO(umesh): prepare file on disk
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

    // TODO(umesh): verify checksum
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

Error FileCommitLogger::startTx(const BlobTxId & txDesc, const std::string & blobName,
        fds_uint64_t & id) {
    size_t sz = sizeof(DmCommitLogEntry) + blobName.length() + 1;
    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    id = clEpoch++;
    DmCommitLogStartEntry * entry = new(clp) DmCommitLogStartEntry(txDesc, id, blobName);
    addEntryToHeader(entry);

    if (0 != msync(clp, entry->length(), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::updateTx(const BlobTxId * txDesc, BlobObjList::const_ptr blobObjList,
        fds_uint64_t & id) {
    return ERR_OK;
}

Error FileCommitLogger::updateTx(const BlobTxId & txDesc, BlobMetaDesc::const_ptr blobMetaDesc,
        fds_uint64_t & id) {
    return ERR_OK;
}

Error FileCommitLogger::commitTx(const BlobTxId & txDesc, fds_uint64_t & id) {
    size_t sz = sizeof(DmCommitLogEntry) + 1;
    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    id = clEpoch++;
    DmCommitLogCommitEntry * entry = new(clp) DmCommitLogCommitEntry(txDesc, id);
    addEntryToHeader(entry);

    if (0 != msync(clp, entry->length(), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::rollbackTx(const BlobTxId & txDesc, fds_uint64_t & id) {
    size_t sz = sizeof(DmCommitLogEntry) + 1;
    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    id = clEpoch++;
    DmCommitLogRollbackEntry * entry = new(clp) DmCommitLogRollbackEntry(txDesc, id);
    addEntryToHeader(entry);

    if (0 != msync(clp, entry->length(), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::purgeTx(const BlobTxId & txDesc, fds_uint64_t & id) {
    size_t sz = sizeof(DmCommitLogEntry) + 1;
    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    id = clEpoch++;
    DmCommitLogPurgeEntry * entry = new(clp) DmCommitLogPurgeEntry(txDesc, id);
    addEntryToHeader(entry);

    if (0 != msync(clp, entry->length(), MS_ASYNC /* MS_SYNC */)) {
        LOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

const DmCommitLogEntry * FileCommitLogger::getNext(const DmCommitLogEntry * rhs) {
    fds_assert(rhs);
    fds_assert(header()->count);
    fds_verify(first() <= rhs && last() >= rhs);

    if (!rhs->next) {
        return 0;   // last entry
    }

    return reinterpret_cast<DmCommitLogEntry *>(addr_ + rhs->next);
}

const DmCommitLogEntry * FileCommitLogger::getEntry(const fds_uint64_t id) {
    const DmCommitLogEntry * ret = getFirst();
    for (; 0 != ret; ret = getNext(ret)) {
        if (id == ret->id) {
            break;
        }
    }
    return ret;
}

}  /* namespace fds */

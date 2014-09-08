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
#include <set>

#include <dm-tvc/CommitLog.h>
#include <DataMgr.h>

#include <PerfTrace.h>

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
        filesize_(filesize), persist_(persist), started_(false), compacting_(ATOMIC_FLAG_INIT) {
    if (filesize_ < MIN_COMMIT_LOG_FILE_SIZE) {
        GLOGWARN << "Commit log size can't be less than 1MB, setting it to 1MB";
        filesize_ = MIN_COMMIT_LOG_FILE_SIZE;
    } else if (filesize_ > MAX_COMMIT_LOG_FILE_SIZE) {
        GLOGWARN << "Commit log size can't be more than 100MB, setting it to 100MB";
        filesize_ = MAX_COMMIT_LOG_FILE_SIZE;
    }

    if (IN_FILE == persist_) {
        cmtLogger_.reset(new FileCommitLogger(filename_, filesize_));
    } else if (IN_MEMORY == persist_) {
        cmtLogger_.reset(new MemoryCommitLogger(filesize_));
    } else {
        // TODO(umesh): instantiate leveldb based cmtLogger_
    }

    const DmCommitLogEntry * entry = cmtLogger_->getLast();
    if (entry) {
        clEpoch = entry->id;
    }

    // populate txMap_
    for (const DmCommitLogEntry * entry = cmtLogger_->getFirst(); 0 != entry;
            entry = cmtLogger_->getNext(entry)) {
        BlobTxId txId(entry->txId);

        TxMap::iterator iter = txMap_.find(txId);

        /*
         * XXX: we cannot do the check below as we need to handle partially
         *      compacted entries.
        fds_verify((TX_START == entry->type && txMap_.end() == iter) ||
                (TX_START != entry->type && txMap_.end() != iter));
         */

        if (txMap_.end() == iter) {
            txMap_[txId].reset(new CommitLogTx());
            txMap_[txId]->txDesc.reset(new BlobTxId(entry->txId));
            iter = txMap_.find(txId);
        }

        iter->second->entries.push_back(entry);

        switch (entry->type) {
            case TX_START: {
                StartTxDetails::const_ptr details =
                        reinterpret_cast<const DmCommitLogStartEntry *>(
                        entry)->getStartTxDetails();
                iter->second->blobMode = details->blobMode;
                iter->second->blobName = details->blobName;
                iter->second->started = true;
            }
            case TX_COMMIT:
                iter->second->committed = true;
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
            case TX_DELETE_BLOB: {
                iter->second->blobDelete = true;
                iter->second->blobVersion =
                        reinterpret_cast<const DmCommitLogDeleteBlobEntry *>\
                        (entry)->blobVersion();
                break;
            }
            case TX_PURGE: {
                iter->second->purged = true;
                break;
            }
            default:
                break;
        }
    }

    // this could be restart, we have to cleanup uncommitted entries from previous run
    std::set<fds_uint64_t> txIds;

    GLOGNORMAL << "Purging commit log of stale uncommitted entries from previous run";
    {
        SCOPEDREAD(lockTxMap_);
        for (auto it : txMap_) {
            if (!it.second->purged && !it.second->rolledback && !it.second->committed) {
                txIds.insert(it.first.getValue());  // candidates
            }
        }
    }

    if (!txIds.empty()) {
        const std::set<fds_uint64_t> & compacted = cmtLogger_->compactLog(txIds);
        fds_assert(compacted.empty());

        for (auto i : txIds) {
            SCOPEDWRITE(lockTxMap_);
            txMap_.erase(BlobTxId(i));
        }
    }

    scheduleCompaction();
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

void DmCommitLog::compactLog(dmCatReq * req) {
    std::set<fds_uint64_t> txIds;

    GLOGNORMAL << "Compacting commit log '" << filename_ << "'";

    {
        SCOPEDREAD(lockTxMap_);
        for (auto it : txMap_) {
            if (it.second->purged || it.second->rolledback) {
                txIds.insert(it.first.getValue());  // candidates
            }
        }
    }

    if (!txIds.empty()) {
        const std::set<fds_uint64_t> & compacted = cmtLogger_->compactLog(txIds);

        for (auto i : compacted) {
            SCOPEDWRITE(lockTxMap_);
            txMap_.erase(BlobTxId(i));
        }
    }

    compacting_.clear();
    if (dataMgr) {
        // we must call markIODone to correctly account for outstanding IOs
        dataMgr->qosCtrl->markIODone(*req);
    }
    delete req;
}

/*
 * operations
 */
// start transaction
Error DmCommitLog::startTx(BlobTxId::const_ptr & txDesc, const std::string & blobName,
        fds_int32_t blobMode) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Starting blob transaction (" << txId << ") for (" << blobName << ")";

    {
        SCOPEDREAD(lockTxMap_);

        TxMap::const_iterator iter = txMap_.find(txId);
        if (txMap_.end() != iter) {
            GLOGWARN << "Blob transaction already exists";
            return ERR_DM_TX_EXISTS;
        }
    }

    StartTxDetails::const_ptr details(new StartTxDetails(blobMode, blobName));
    DmCommitLogEntry * entry = 0;
    Error rc = cmtLogger_->startTx(txDesc, details, entry);
    if (!rc.ok()) {
        GLOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    if (cmtLogger_->hasReachedSizeThreshold() && !compacting_.test_and_set()) {
        scheduleCompaction();
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId].reset(new CommitLogTx());
    txMap_[txId]->txDesc = txDesc;
    txMap_[txId]->blobName = blobName;
    txMap_[txId]->blobMode = blobMode;
    txMap_[txId]->entries.push_back(entry);
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

    GLOGDEBUG << "Update blob for transaction (" << txId << ")";

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    DmCommitLogEntry* entry = 0;
    rc = cmtLogger_->updateTx(txDesc, blobData, entry);
    if (!rc.ok()) {
        GLOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    if (cmtLogger_->hasReachedSizeThreshold() && !compacting_.test_and_set()) {
        scheduleCompaction();
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId]->entries.push_back(entry);

    upsertBlobData(*txMap_[txId], blobData);

    return ERR_OK;
}

// delete blob
Error DmCommitLog::deleteBlob(BlobTxId::const_ptr & txDesc, const blob_version_t blobVersion) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Delete blob in transaction (" << txId << ")";

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    DeleteBlobDetails::const_ptr details(new DeleteBlobDetails(blobVersion));
    DmCommitLogEntry * entry = 0;
    rc = cmtLogger_->deleteBlob(txDesc, details, entry);
    if (!rc.ok()) {
        GLOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    if (cmtLogger_->hasReachedSizeThreshold() && !compacting_.test_and_set()) {
        scheduleCompaction();
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId]->entries.push_back(entry);
    txMap_[txId]->blobDelete = true;
    txMap_[txId]->blobVersion = blobVersion;

    return ERR_OK;
}

// commit transaction
CommitLogTx::const_ptr DmCommitLog::commitTx(BlobTxId::const_ptr & txDesc, Error & status) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Commiting blob transaction " << txId;

    status = validateSubsequentTx(txId);
    if (!status.ok()) {
        return 0;
    }

    DmCommitLogEntry * entry = 0;
    status = cmtLogger_->commitTx(txDesc, entry);
    if (!status.ok()) {
        GLOGERROR << "Failed to save blob transaction error=(" << status << ")";
        return txMap_[txId];
    }

    if (cmtLogger_->hasReachedSizeThreshold() && !compacting_.test_and_set()) {
        scheduleCompaction();
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId]->entries.push_back(entry);
    txMap_[txId]->committed = true;

    return txMap_[txId];
}

// rollback transaction
Error DmCommitLog::rollbackTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Rollback blob transaction " << txId;

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    DmCommitLogEntry * entry = 0;
    rc = cmtLogger_->rollbackTx(txDesc, entry);
    if (!rc.ok()) {
        GLOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    if (cmtLogger_->hasReachedSizeThreshold() && !compacting_.test_and_set()) {
        scheduleCompaction();
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId]->entries.push_back(entry);
    txMap_[txId]->rolledback = true;

    return rc;
}

// purge transaction
Error DmCommitLog::purgeTx(BlobTxId::const_ptr  & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Purge blob transaction " << txId;

    {
        SCOPEDREAD(lockTxMap_);

        TxMap::iterator iter = txMap_.find(txId);
        if (txMap_.end() == iter) {
            GLOGERROR << "Blob transaction not started";
            return ERR_DM_TX_NOT_STARTED;
        }
        fds_assert(txId == *(iter->second->txDesc));

        if (!iter->second->rolledback && !iter->second->committed) {
            GLOGERROR << "Blob transaction active, can not be purged";
            return ERR_DM_TX_ACTIVE;
        }
    }

    DmCommitLogEntry * entry = 0;
    Error rc = cmtLogger_->purgeTx(txDesc, entry);
    if (!rc.ok()) {
        GLOGERROR << "Failed to save blob transaction error=(" << rc << ")";
        return rc;
    }

    if (cmtLogger_->hasReachedSizeThreshold() && !compacting_.test_and_set()) {
        scheduleCompaction();
    }

    SCOPEDWRITE(lockTxMap_);

    txMap_[txId]->entries.push_back(entry);
    txMap_[txId]->purged = true;

    return ERR_OK;
}

// get transaction
CommitLogTx::const_ptr DmCommitLog::getTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_assert(cmtLogger_);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Get blob transaction " << txId;

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
        GLOGERROR << "Blob transaction not started";
        return ERR_DM_TX_NOT_STARTED;
    }
    fds_assert(txId == *(iter->second->txDesc));

    if (iter->second->rolledback) {
        GLOGERROR << "Blob transaction already rolled back";
        return ERR_DM_TX_ROLLEDBACK;
    } else if (iter->second->committed) {
        GLOGERROR << "Blob transaction already committed";
        return ERR_DM_TX_COMMITTED;
    }

    return ERR_OK;
}

fds_bool_t DmCommitLog::isPendingTx(const fds_uint64_t tsNano /* = util::getTimeStampNanos() */) {
    fds_bool_t ret = false;
    SCOPEDREAD(lockTxMap_);
    for (auto it : txMap_) {
        if (it.second->started && (!it.second->rolledback && !it.second->committed)) {
            if (it.second->entries[0]->timestamp <= tsNano) {
                ret = true;
            }
        }
    }

    return ret;
}

void DmCommitLog::scheduleCompaction() {
    dmCatReq * compactReq = new dmCatReq();
    compactReq->io_type = FDS_DM_PURGE_COMMIT_LOG;
    compactReq->proc = std::bind(&DmCommitLog::compactLog, this, std::placeholders::_1);
    if (dataMgr) {
        dataMgr->enqueueMsg(FdsDmSysTaskId, compactReq);
    }
}

template<typename T>
const std::string DmCommitLogEntry::createPayload(boost::shared_ptr<const T> val) {
    boost::scoped_ptr<serialize::Serializer> s(serialize::getMemSerializer());
    val->write(s.get());
    return s->getBufferAsString();
}

uint32_t StartTxDetails::write(serialize::Serializer*  s) const {
    fds_assert(s);
    uint32_t sz = s->writeI32(blobMode);
    sz += s->writeString(blobName);
    return sz;
}

uint32_t StartTxDetails::read(serialize::Deserializer* d) {
    fds_assert(d);
    uint32_t sz = d->readI32(blobMode);
    sz += d->readString(blobName);
    return sz;
}

uint32_t DeleteBlobDetails::write(serialize::Serializer* s) const {
    fds_assert(s);
    return s->writeI64(blobVersion);
}

uint32_t DeleteBlobDetails::read(serialize::Deserializer* d) {
    fds_assert(d);
    return d->readI64(blobVersion);
}

void FileCommitLogger::syncHeader() {
    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(header())),
            sizeof(EntriesHeader), MS_SYNC /* MS_ASYNC */)) {
        GLOGWARN << "Failed to write commit log header";
    }
}

void FileCommitLogger::addEntryToHeader(DmCommitLogEntry * entry, bool sync /* = true */) {
    fds_uint32_t offset = entryOffset(entry);
    last()->next = offset;
    header()->last = offset;
    ++(header()->count);

    if (sync) {
        syncHeader();
    }
}

void FileCommitLogger::removeEntry(bool sync /* = true */) {
    const DmCommitLogEntry * fe = first();

    // if count is 0 then no need to move first, as the first and the last were
    // pointing to the only entry in commit log and count is set to 0 now.
    --(header()->count);
    if (header()->count) {
        header()->first = entryOffset(getNextInternal(fe));
    }

    if (sync) {
        syncHeader();
    }
}

FileCommitLogger::FileCommitLogger(const std::string & filename, fds_uint32_t filesize)
        : filename_(filename), filesize_(filesize), fd_(-1), prot_(PROT_READ | PROT_WRITE),
        flags_(MAP_PRIVATE | MAP_ANONYMOUS), addr_(0), lockLogFile_("commit log file lock"),
        logCtx(COMMIT_LOG_WRITE, 0, filename) {
    bool create = true;

    if (!filename.empty()) {
        GLOGNORMAL << "Writing commit log to file '" << filename << "'";

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

Error FileCommitLogger::startTx(BlobTxId::const_ptr & txDesc, StartTxDetails::const_ptr & details,
        DmCommitLogEntry* & entry) {
    const std::string & str = DmCommitLogEntry::createPayload(details);
    size_t sz = sizeof(DmCommitLogEntry) + str.length() + 1;

    FDSGUARD(lockLogFile_);

    SCOPED_PERF_TRACEPOINT_CTX_DEBUG(logCtx);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    entry = new(clp) DmCommitLogStartEntry(txDesc, clEpoch++, str);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_SYNC /* MS_ASYNC */)) {
        GLOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::updateTx(BlobTxId::const_ptr & txDesc, BlobObjList::const_ptr blobObjList,
        DmCommitLogEntry* & entry) {
    const std::string & str = DmCommitLogEntry::createPayload(blobObjList);
    size_t sz = sizeof(DmCommitLogEntry) + str.length() + 1;

    FDSGUARD(lockLogFile_);

    SCOPED_PERF_TRACEPOINT_CTX_DEBUG(logCtx);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    entry = new(clp) DmCommitLogUpdateObjListEntry(txDesc, clEpoch++, str);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_SYNC /* MS_ASYNC */)) {
        GLOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::updateTx(BlobTxId::const_ptr & txDesc, MetaDataList::const_ptr metaDataList,
        DmCommitLogEntry* & entry) {
    const std::string & str = DmCommitLogEntry::createPayload(metaDataList);
    size_t sz = sizeof(DmCommitLogEntry) + str.length() + 1;

    FDSGUARD(lockLogFile_);

    SCOPED_PERF_TRACEPOINT_CTX_DEBUG(logCtx);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    entry = new(clp) DmCommitLogUpdateObjMetaEntry(txDesc, clEpoch++, str);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_SYNC /* MS_ASYNC */)) {
        GLOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::deleteBlob(BlobTxId::const_ptr & txDesc,
        DeleteBlobDetails::const_ptr &details, DmCommitLogEntry* & entry) {
    const std::string & str = DmCommitLogEntry::createPayload(details);
    size_t sz = sizeof(DmCommitLogEntry) + str.length() + 1;

    FDSGUARD(lockLogFile_);

    SCOPED_PERF_TRACEPOINT_CTX_DEBUG(logCtx);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    entry = new(clp) DmCommitLogDeleteBlobEntry(txDesc, clEpoch++, str);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_SYNC /* MS_ASYNC */)) {
        GLOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::commitTx(BlobTxId::const_ptr & txDesc, DmCommitLogEntry* & entry) {
    size_t sz = sizeof(DmCommitLogEntry);

    FDSGUARD(lockLogFile_);

    SCOPED_PERF_TRACEPOINT_CTX_DEBUG(logCtx);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    entry = new(clp) DmCommitLogCommitEntry(txDesc, clEpoch++);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_SYNC /* MS_ASYNC */)) {
        GLOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::rollbackTx(BlobTxId::const_ptr & txDesc, DmCommitLogEntry* & entry) {
    size_t sz = sizeof(DmCommitLogEntry);

    FDSGUARD(lockLogFile_);

    SCOPED_PERF_TRACEPOINT_CTX_DEBUG(logCtx);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    entry = new(clp) DmCommitLogRollbackEntry(txDesc, clEpoch++);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_SYNC /* MS_ASYNC */)) {
        GLOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

Error FileCommitLogger::purgeTx(BlobTxId::const_ptr & txDesc, DmCommitLogEntry* & entry) {
    size_t sz = sizeof(DmCommitLogEntry);

    FDSGUARD(lockLogFile_);

    SCOPED_PERF_TRACEPOINT_CTX_DEBUG(logCtx);

    char * clp = commitLogAlloc(sz);
    if (!clp) {
        return ERR_DM_MAX_CL_ENTRIES;
    }

    entry = new(clp) DmCommitLogPurgeEntry(txDesc, clEpoch++);
    addEntryToHeader(entry);

    if (0 != msync(reinterpret_cast<void *>(FDS_PAGE_START_ADDR(entry)), FDS_PAGE_OFFSET(entry) +
            entry->length(), MS_SYNC /* MS_ASYNC */)) {
        GLOGWARN << "Failed to write commit log entry";
    }

    return ERR_OK;
}

const DmCommitLogEntry * FileCommitLogger::getNextInternal(const DmCommitLogEntry * rhs) {
    fds_assert(rhs);
    fds_assert(header()->count);

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

const std::set<fds_uint64_t>
FileCommitLogger::compactLog(const std::set<fds_uint64_t> & candidates) {
    std::set<fds_uint64_t> ret;

    while (ret.size() < candidates.size()) {
        FDSGUARD(lockLogFile_);

        SCOPED_PERF_TRACEPOINT_CTX_DEBUG(logCtx);

        const DmCommitLogEntry * entry = getFirstInternal();
        if (!entry) {
            GLOGDEBUG << "Compacted all entries. Commit log is now empty";
            break;
        }

        std::set<fds_uint64_t>::const_iterator pos = candidates.find(entry->txId);
        if (candidates.end() == pos) {
            GLOGNORMAL << "Found active with id '" << entry->id << "'";
            break;
        }

        removeEntry(false);

        if (TX_PURGE == entry->type || TX_ROLLBACK == entry->type) {
            ret.insert(entry->txId);
        }
    }

    syncHeader();

    return ret;
}

bool FileCommitLogger::hasReachedSizeThreshold() {
    FDSGUARD(lockLogFile_);

    if (!header()->count) {
        return false;
    } else if (header()->first <= header()->last) {
        // used is greater than threshold
        return ((filesize_ * PERCENT_SIZE_THRESHOLD) / 100 <
                (header()->last - header()->first));
    }

    // unused is less than (100 - threshold)
    return ((header()->first - header()->last) <
            (filesize_ * (100 - PERCENT_SIZE_THRESHOLD)) / 100);
}

}  /* namespace fds */

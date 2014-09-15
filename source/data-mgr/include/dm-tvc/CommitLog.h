/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_

#include <unistd.h>
#include <string.h>
#include <set>

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <atomic>

#include <fds_error.h>
#include <fds_module.h>
#include <util/timeutils.h>
#include <concurrency/RwLock.h>
#include <concurrency/Mutex.h>
#include <serialize.h>
#include <blob/BlobTypes.h>
#include <DmBlobTypes.h>
#include <FdsCrypto.h>
#include <PerfTypes.h>
#include <boost/scoped_ptr.hpp>
#include <DmIoReq.h>

namespace fds {

const unsigned DEFAULT_COMMIT_LOG_FILE_SIZE = 5 * 1024 * 1024;
const unsigned MIN_COMMIT_LOG_FILE_SIZE = 1 * 1024 * 1024;
const unsigned MAX_COMMIT_LOG_FILE_SIZE = 100 * 1024 * 1024;

struct DmCommitLogEntry;    // forward declaration

// commit log transaction details
struct CommitLogTx {
    typedef boost::shared_ptr<CommitLogTx> ptr;
    typedef boost::shared_ptr<const CommitLogTx> const_ptr;

    BlobTxId::const_ptr txDesc;
    std::string blobName;
    fds_int32_t blobMode;

    bool started;
    bool committed;
    bool rolledback;
    bool blobDelete;
    bool purged;

    std::vector<const DmCommitLogEntry *> entries;

    BlobObjList::ptr blobObjList;
    MetaDataList::ptr metaDataList;

    blob_version_t blobVersion;

    CommitLogTx() : txDesc(0), blobMode(0), started(false), committed(false), rolledback(false),
            blobDelete(false), purged(false), blobObjList(0), metaDataList(0),
            blobVersion(blob_version_invalid) {}
};

typedef std::unordered_map<BlobTxId, CommitLogTx::ptr> TxMap;

class DmCommitLogger;

/**
 * Manages pending DM catalog updates and commits. Used
 * for 2-phase commit operations. This management
 * includes the on-disk log and the in-memory state.
 */
class DmCommitLog : public Module {
  public:
    // types
    typedef enum {
        IN_FILE,
        IN_DB,
        IN_MEMORY
    } PersistenceType;

    typedef boost::shared_ptr<DmCommitLog> ptr;
    typedef boost::shared_ptr<const DmCommitLog> const_ptr;

    // ctor & dtor
    explicit DmCommitLog(const std::string &modName, const std::string & filename,
            fds_uint32_t filesize = DEFAULT_COMMIT_LOG_FILE_SIZE,
            PersistenceType persist = IN_MEMORY);
    ~DmCommitLog();

    // module overrides
    int  mod_init(SysParams const *const param) override;
    void mod_startup() override;
    void mod_shutdown() override;

    void compactLog(dmCatReq * req);

    /*
     * operations
     */
    // start transaction
    Error startTx(BlobTxId::const_ptr & txDesc, const std::string & blobName,
            fds_int32_t blobMode);

    // update blob data (T can be BlobObjList or MetaDataList)
    template<typename T>
    Error updateTx(BlobTxId::const_ptr & txDesc, boost::shared_ptr<const T> & blobData);

    // delete blob
    Error deleteBlob(BlobTxId::const_ptr & txDesc, const blob_version_t blobVersion);

    // commit transaction
    CommitLogTx::const_ptr commitTx(BlobTxId::const_ptr & txDesc, Error & status);

    // rollback transaction
    Error rollbackTx(BlobTxId::const_ptr & txDesc);

    // purge transaction
    Error purgeTx(BlobTxId::const_ptr & txDesc);

    // get transaction
    CommitLogTx::const_ptr getTx(BlobTxId::const_ptr & txDesc);

    // check if any transaction is pending from before the given time
    fds_bool_t isPendingTx(const fds_uint64_t tsNano = util::getTimeStampNanos());

  private:
    TxMap txMap_;    // in-memory state
    fds_rwlock lockTxMap_;

    std::string filename_;
    fds_uint32_t filesize_;
    PersistenceType persist_;
    bool started_;
    boost::shared_ptr<DmCommitLogger> cmtLogger_;
    std::atomic_flag compacting_;

    // Methods
    Error validateSubsequentTx(const BlobTxId & txId);

    void scheduleCompaction();

    void upsertBlobData(CommitLogTx & tx, boost::shared_ptr<const BlobObjList> & data) {
        if (tx.blobObjList) {
            tx.blobObjList->merge(*data);
        } else {
            tx.blobObjList.reset(new BlobObjList(*data));
        }
    }

    void upsertBlobData(CommitLogTx & tx, boost::shared_ptr<const MetaDataList> & data) {
        if (tx.metaDataList) {
            tx.metaDataList->merge(*data);
        } else {
            tx.metaDataList.reset(new MetaDataList(*data));
        }
    }
};

typedef enum {
    TX_UNKNOWN,
    TX_START,
    TX_UPDATE_OBJLIST,
    TX_UPDATE_OBJMETA,
    TX_DELETE_BLOB,
    TX_ROLLBACK,
    TX_COMMIT,
    TX_PURGE
} CommitLogEntryType;

// base struct for commit log entry
struct DmCommitLogEntry {
    CommitLogEntryType type;
    fds_uint64_t id;
    fds_uint64_t txId;
    fds_uint64_t timestamp;
    fds_uint32_t next;

    fds_uint32_t len;
    char payload[];     // should be last data member of structure

    DmCommitLogEntry(CommitLogEntryType type_, BlobTxId::const_ptr & txDesc, fds_uint64_t id_,
            fds_uint32_t len_ = 0, const char * payload_ = 0) : type(type_), id(id_),
            txId(txDesc->getValue()), timestamp(util::getTimeStampNanos()), next(0), len(len_) {
        if (len && payload_) {
            memcpy(payload, payload_, len);
        }
    }

    inline fds_uint32_t length() const {
        return sizeof(DmCommitLogEntry) + len;
    }

    template<typename T>
    static const std::string createPayload(boost::shared_ptr<const T> val);

  protected:
    template<typename T>
    boost::shared_ptr<const T> getDetails() const {
        const std::string str(payload, len + 1);
        boost::scoped_ptr<serialize::Deserializer> d(serialize::getMemDeserializer(str));
        boost::shared_ptr<T> ret(new T());
        ret->read(d.get());
        return ret;
    }
};

struct StartTxDetails : serialize::Serializable {
    typedef boost::shared_ptr<StartTxDetails> ptr;
    typedef boost::shared_ptr<const StartTxDetails> const_ptr;

    fds_int32_t blobMode;
    std::string blobName;

    StartTxDetails() : blobMode(0) {}
    StartTxDetails(fds_int32_t blobMode_, const std::string & blobName_) : blobMode(blobMode_),
            blobName(blobName_) {}

    uint32_t virtual write(serialize::Serializer*  s) const;
    uint32_t virtual read(serialize::Deserializer* d);
};

struct DeleteBlobDetails : serialize::Serializable {
    typedef boost::shared_ptr<DeleteBlobDetails> ptr;
    typedef boost::shared_ptr<const DeleteBlobDetails> const_ptr;

    blob_version_t blobVersion;

    DeleteBlobDetails() : blobVersion(blob_version_invalid) {}
    explicit DeleteBlobDetails(blob_version_t ver) : blobVersion(ver) {}

    uint32_t virtual write(serialize::Serializer*  s) const;
    uint32_t virtual read(serialize::Deserializer* d);
};

// start transaction
struct DmCommitLogStartEntry : DmCommitLogEntry {
    DmCommitLogStartEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id,
            const std::string & blobDetails)
            : DmCommitLogEntry(TX_START, txDesc, id, blobDetails.length() + 1,
            blobDetails.c_str()) {}

    inline std::string blobName() const {
        return getStartTxDetails()->blobName;
    }

    inline fds_int32_t blobMode() const {
        return getStartTxDetails()->blobMode;
    }

    inline StartTxDetails::const_ptr getStartTxDetails() const {
        fds_assert(TX_START == type);
        return getDetails<StartTxDetails>();
    }
};

// commit transaction
struct DmCommitLogCommitEntry : DmCommitLogEntry {
    DmCommitLogCommitEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id) :
            DmCommitLogEntry(TX_COMMIT, txDesc, id) {}
};

// rollback transaction
struct DmCommitLogRollbackEntry : DmCommitLogEntry {
    DmCommitLogRollbackEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id) :
            DmCommitLogEntry(TX_ROLLBACK, txDesc, id) {}
};

// purge transaction, read for compaction/ gc
struct DmCommitLogPurgeEntry : DmCommitLogEntry {
    DmCommitLogPurgeEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id) :
            DmCommitLogEntry(TX_PURGE, txDesc, id) {}
};

// delete blob
struct DmCommitLogDeleteBlobEntry : DmCommitLogEntry {
    DmCommitLogDeleteBlobEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id,
            const std::string & blobDetails) :
        DmCommitLogEntry(TX_DELETE_BLOB, txDesc, id) {}

    inline blob_version_t blobVersion() const {
        return getDeleteBlobDetails()->blobVersion;
    }

    inline DeleteBlobDetails::const_ptr getDeleteBlobDetails() const {
        fds_assert(TX_DELETE_BLOB == type);
        return getDetails<DeleteBlobDetails>();
    }
};

// update blob object list
struct DmCommitLogUpdateObjListEntry : DmCommitLogEntry {
    DmCommitLogUpdateObjListEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id,
            const std::string & blobObjList) : DmCommitLogEntry(TX_UPDATE_OBJLIST, txDesc, id,
            blobObjList.length() + 1, blobObjList.c_str()) {}

    inline boost::shared_ptr<const BlobObjList> blobObjList() const {
        fds_assert(TX_UPDATE_OBJLIST == type);
        return getDetails<BlobObjList>();
    }
};

// update blob object meta data
struct DmCommitLogUpdateObjMetaEntry : DmCommitLogEntry {
    DmCommitLogUpdateObjMetaEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id,
            const std::string & metaDataList) : DmCommitLogEntry(TX_UPDATE_OBJMETA, txDesc, id,
            metaDataList.length() + 1, metaDataList.c_str()) {}

    inline boost::shared_ptr<const MetaDataList> metaDataList() const {
        fds_assert(TX_UPDATE_OBJMETA == type);
        return getDetails<MetaDataList>();
    }
};

class DmCommitLogger {
  public:
    typedef boost::shared_ptr<DmCommitLogger> ptr;
    typedef boost::shared_ptr<const DmCommitLogger> const_ptr;

    static const fds_uint32_t PERCENT_SIZE_THRESHOLD = 70;

    virtual Error startTx(BlobTxId::const_ptr & txDesc, StartTxDetails::const_ptr & details,
            DmCommitLogEntry* & entry) = 0;

    virtual Error updateTx(BlobTxId::const_ptr & txDesc, BlobObjList::const_ptr blobObjList,
            DmCommitLogEntry* & entry) = 0;

    virtual Error updateTx(BlobTxId::const_ptr & txDesc, MetaDataList::const_ptr metaDataList,
            DmCommitLogEntry* & entry) = 0;

    virtual Error deleteBlob(BlobTxId::const_ptr & txDesc, DeleteBlobDetails::const_ptr & details,
            DmCommitLogEntry* & entry) = 0;

    virtual Error commitTx(BlobTxId::const_ptr & txDesc, DmCommitLogEntry* & entry) = 0;

    virtual Error rollbackTx(BlobTxId::const_ptr & txDesc, DmCommitLogEntry* & entry) = 0;

    virtual Error purgeTx(BlobTxId::const_ptr & txDesc, DmCommitLogEntry* & entry) = 0;

    virtual const DmCommitLogEntry * getFirst() = 0;

    virtual const DmCommitLogEntry * getLast() = 0;

    virtual const DmCommitLogEntry * getNext(const DmCommitLogEntry * rhs) = 0;

    virtual const DmCommitLogEntry * getEntry(const fds_uint64_t id) = 0;

    virtual const std::set<fds_uint64_t> compactLog(const std::set<fds_uint64_t> & candidates) = 0;

    virtual bool hasReachedSizeThreshold(fds_uint16_t threshold) = 0;
};

class FileCommitLogger : public DmCommitLogger {
  public:
    struct EntriesHeader {
        fds_uint32_t count;
        fds_uint32_t first;
        fds_uint32_t last;
        byte digest[hash::Sha1::numDigestBytes];
    };

    FileCommitLogger(const std::string & filename, fds_uint32_t filesize);
    virtual ~FileCommitLogger();

    virtual Error startTx(BlobTxId::const_ptr & txDesc, StartTxDetails::const_ptr & details,
            DmCommitLogEntry* & entry) override;

    virtual Error updateTx(BlobTxId::const_ptr & txDesc, BlobObjList::const_ptr blobObjList,
            DmCommitLogEntry* & entry) override;

    virtual Error updateTx(BlobTxId::const_ptr & txDesc, MetaDataList::const_ptr metaDataList,
            DmCommitLogEntry* & entry) override;

    virtual Error deleteBlob(BlobTxId::const_ptr & txDesc, DeleteBlobDetails::const_ptr & details,
            DmCommitLogEntry* & entry) override;

    virtual Error commitTx(BlobTxId::const_ptr & txDesc, DmCommitLogEntry* & entry) override;

    virtual Error rollbackTx(BlobTxId::const_ptr & txDesc, DmCommitLogEntry* & entry) override;

    virtual Error purgeTx(BlobTxId::const_ptr & txDesc, DmCommitLogEntry* & entry) override;

    virtual const DmCommitLogEntry * getFirst() override {
        FDSGUARD(lockLogFile_);
        return getFirstInternal();
    }

    virtual const DmCommitLogEntry * getLast() override {
        FDSGUARD(lockLogFile_);
        return getLastInternal();
    }

    virtual const DmCommitLogEntry * getNext(const DmCommitLogEntry * rhs) override {
        FDSGUARD(lockLogFile_);
        return getNextInternal(rhs);
    }

    virtual const DmCommitLogEntry * getEntry(const fds_uint64_t id) override;

    virtual const std::set<fds_uint64_t> compactLog(const std::set<fds_uint64_t> &
            candidates) override;

    virtual bool hasReachedSizeThreshold(fds_uint16_t threshold) override;

  protected:
    inline EntriesHeader * header() {
        return reinterpret_cast<EntriesHeader *>(addr_);
    }

    inline char * data() {
        return addr_ + sizeof(EntriesHeader);
    }

    inline fds_uint32_t entryOffset(const DmCommitLogEntry * rhs) {
        return reinterpret_cast<const char *>(rhs) - addr_;
    }

    inline DmCommitLogEntry * first() {
        return reinterpret_cast<DmCommitLogEntry *>(addr_ + header()->first);
    }

    inline DmCommitLogEntry * last() {
        return reinterpret_cast<DmCommitLogEntry *>(addr_ + header()->last);
    }

    char * commitLogAlloc(const size_t sz);

    void addEntryToHeader(DmCommitLogEntry * entry, bool sync = true);

    void removeEntry(bool sync = true);  // removes first entry

    inline const DmCommitLogEntry * getFirstInternal() {
        return header()->count ? first() : 0;
    }

    inline const DmCommitLogEntry * getLastInternal() {
        return header()->count ? last() : 0;
    }

    void syncHeader();

    const DmCommitLogEntry * getNextInternal(const DmCommitLogEntry * rhs);

  private:
    const std::string filename_;
    const fds_uint32_t filesize_;
    int fd_;
    int prot_;
    int flags_;

    char * addr_;
    fds_mutex lockLogFile_;
    PerfContext logCtx;
};

class MemoryCommitLogger : public FileCommitLogger {
  public:
    explicit MemoryCommitLogger(fds_uint32_t filesize = DEFAULT_COMMIT_LOG_FILE_SIZE)
            : FileCommitLogger("", filesize) {}
    virtual ~MemoryCommitLogger() {}
};

}  /* namespace fds */

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_

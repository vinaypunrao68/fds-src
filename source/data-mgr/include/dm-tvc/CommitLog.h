/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_

#include <unistd.h>
#include <string.h>

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>

#include <fds_error.h>
#include <fds_module.h>
#include <concurrency/RwLock.h>
#include <serialize.h>
#include <blob/BlobTypes.h>
#include <DmBlobTypes.h>
#include <FdsCrypto.h>
#include <PerfTrace.h>

namespace fds {

const unsigned DEFAULT_COMMIT_LOG_FILE_SIZE = 5 * 1024 * 1024;

// commit log transaction details
struct CommitLogTx {
    typedef boost::shared_ptr<CommitLogTx> ptr;
    typedef boost::shared_ptr<const CommitLogTx> const_ptr;

    BlobTxId::const_ptr txDesc;
    std::string blobName;

    bool started;
    bool commited;
    bool rolledback;

    std::vector<fds_uint64_t> entries;

    BlobObjList::ptr blobObjList;
    MetaDataList::ptr metaDataList;

    CommitLogTx() : txDesc(0), started(false), commited(false), rolledback(false),
            blobObjList(0), metaDataList(0) {}
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

    /*
     * operations
     */
    // start transaction
    Error startTx(BlobTxId::const_ptr & txDesc, const std::string & blobName);

    // update blob data (T can be BlobObjList or MetaDataList)
    template<typename T>
    Error updateTx(BlobTxId::const_ptr & txDesc, boost::shared_ptr<const T> & blobData);

    // commit transaction
    CommitLogTx::const_ptr commitTx(BlobTxId::const_ptr & txDesc, Error & status);

    // rollback transaction
    Error rollbackTx(BlobTxId::const_ptr & txDesc);

    // purge transaction
    Error purgeTx(BlobTxId::const_ptr & txDesc);

    // get transaction
    CommitLogTx::const_ptr getTx(BlobTxId::const_ptr & txDesc);

  private:
    TxMap txMap_;    // in-memory state
    fds_rwlock lockTxMap_;

    std::string filename_;
    fds_uint32_t filesize_;
    PersistenceType persist_;
    bool started_;
    PerfContext logCtx;
    boost::shared_ptr<DmCommitLogger> cmtLogger_;

    // Methods
    Error validateSubsequentTx(const BlobTxId & txId);

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
    TX_ROLLBACK,
    TX_COMMIT,
    TX_PURGE
} CommitLogEntryType;

// base struct for commit log entry
struct DmCommitLogEntry {
    CommitLogEntryType type;
    fds_uint64_t id;
    fds_uint64_t txId;
    fds_uint32_t next;

    char payload[];     // should be last data member of structure

    DmCommitLogEntry(CommitLogEntryType type_, BlobTxId::const_ptr & txDesc, fds_uint64_t id_,
            const char * payload_ = 0) : type(type_), id(id_), txId(txDesc->getValue()), next(0) {
        if (payload_) {
            strcpy(payload, payload_);  //NOLINT
        } else {
            *payload = 0;
        }
    }

    inline fds_uint32_t length() const {
        return sizeof(DmCommitLogEntry) + strlen(payload) + 1;
    }

    template<typename T>
    static const std::string createPayload(boost::shared_ptr<const T> val);

  protected:
    template<typename T>
    boost::shared_ptr<const T> getDetails() const {
        boost::scoped_ptr<serialize::Deserializer> d(serialize::getMemDeserializer(payload));
        boost::shared_ptr<T> ret(new T());
        ret->read(d.get());
        return ret;
    }
};

// start transaction
struct DmCommitLogStartEntry : DmCommitLogEntry {
    DmCommitLogStartEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id,
            const std::string & blobName)
            : DmCommitLogEntry(TX_START, txDesc, id, blobName.c_str()) {}

    const char * blobName() const {
        fds_assert(TX_START == type);
        return payload;
    }
};

// commit transaction
struct DmCommitLogCommitEntry : DmCommitLogEntry {
    explicit DmCommitLogCommitEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id) :
            DmCommitLogEntry(TX_COMMIT, txDesc, id) {}
};

// rollback transaction
struct DmCommitLogRollbackEntry : DmCommitLogEntry {
    explicit DmCommitLogRollbackEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id) :
            DmCommitLogEntry(TX_ROLLBACK, txDesc, id) {}
};

// purge transaction, read for compaction/ gc
struct DmCommitLogPurgeEntry : DmCommitLogEntry {
    explicit DmCommitLogPurgeEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id) :
            DmCommitLogEntry(TX_PURGE, txDesc, id) {}
};

// update blob object list
struct DmCommitLogUpdateObjListEntry : DmCommitLogEntry {
    DmCommitLogUpdateObjListEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id,
            const std::string & blobObjList) : DmCommitLogEntry(TX_UPDATE_OBJLIST, txDesc, id,
            blobObjList.c_str()) {}

    inline boost::shared_ptr<const BlobObjList> blobObjList() const {
        fds_assert(TX_UPDATE_OBJLIST == type);
        return getDetails<BlobObjList>();
    }
};

// update blob object meta data
struct DmCommitLogUpdateObjMetaEntry : DmCommitLogEntry {
    DmCommitLogUpdateObjMetaEntry(BlobTxId::const_ptr & txDesc, fds_uint64_t id,
            const std::string & metaDataList) : DmCommitLogEntry(TX_UPDATE_OBJMETA, txDesc, id,
            metaDataList.c_str()) {}

    inline boost::shared_ptr<const MetaDataList> metaDataList() const {
        fds_assert(TX_UPDATE_OBJMETA == type);
        return getDetails<MetaDataList>();
    }
};

class DmCommitLogger {
  public:
    typedef boost::shared_ptr<DmCommitLogger> ptr;
    typedef boost::shared_ptr<const DmCommitLogger> const_ptr;

    virtual Error startTx(BlobTxId::const_ptr & txDesc, const std::string & blobName,
            fds_uint64_t & id) = 0;

    virtual Error updateTx(BlobTxId::const_ptr & txDesc, BlobObjList::const_ptr blobObjList,
            fds_uint64_t & id) = 0;

    virtual Error updateTx(BlobTxId::const_ptr & txDesc, MetaDataList::const_ptr metaDataList,
            fds_uint64_t & id) = 0;

    virtual Error commitTx(BlobTxId::const_ptr & txDesc, fds_uint64_t & id) = 0;

    virtual Error rollbackTx(BlobTxId::const_ptr & txDesc, fds_uint64_t & id) = 0;

    virtual Error purgeTx(BlobTxId::const_ptr & txDesc, fds_uint64_t & id) = 0;

    virtual const DmCommitLogEntry * getFirst() = 0;

    virtual const DmCommitLogEntry * getLast() = 0;

    virtual const DmCommitLogEntry * getNext(const DmCommitLogEntry * rhs) = 0;

    virtual const DmCommitLogEntry * getEntry(const fds_uint64_t id) = 0;
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

    virtual Error startTx(BlobTxId::const_ptr & txDesc, const std::string & blobName,
            fds_uint64_t & id) override;

    virtual Error updateTx(BlobTxId::const_ptr & txDesc, BlobObjList::const_ptr blobObjList,
            fds_uint64_t & id) override;

    virtual Error updateTx(BlobTxId::const_ptr & txDesc, MetaDataList::const_ptr metaDataList,
            fds_uint64_t & id) override;

    virtual Error commitTx(BlobTxId::const_ptr & txDesc, fds_uint64_t & id) override;

    virtual Error rollbackTx(BlobTxId::const_ptr & txDesc, fds_uint64_t & id) override;

    virtual Error purgeTx(BlobTxId::const_ptr & txDesc, fds_uint64_t & id) override;

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

  protected:
    inline EntriesHeader * header() {
        return reinterpret_cast<EntriesHeader *>(addr_);
    }

    inline char * data() {
        return addr_ + sizeof(EntriesHeader);
    }

    inline fds_uint32_t entryOffset(DmCommitLogEntry * rhs) {
        return reinterpret_cast<char *>(rhs) - addr_;
    }

    inline DmCommitLogEntry * first() {
        return reinterpret_cast<DmCommitLogEntry *>(addr_ + header()->first);
    }

    inline DmCommitLogEntry * last() {
        return reinterpret_cast<DmCommitLogEntry *>(addr_ + header()->last);
    }

    char * commitLogAlloc(const size_t sz);

    void addEntryToHeader(DmCommitLogEntry * entry);

    inline const DmCommitLogEntry * getFirstInternal() {
        return header()->count ? first() : 0;
    }

    inline const DmCommitLogEntry * getLastInternal() {
        return header()->count ? last() : 0;
    }

    const DmCommitLogEntry * getNextInternal(const DmCommitLogEntry * rhs);

  private:
    const std::string filename_;
    const fds_uint32_t filesize_;
    int fd_;
    int prot_;
    int flags_;

    char * addr_;
    fds_mutex lockLogFile_;
};

class MemoryCommitLogger : public FileCommitLogger {
  public:
    MemoryCommitLogger() : FileCommitLogger("", DEFAULT_COMMIT_LOG_FILE_SIZE) {}
    virtual ~MemoryCommitLogger() {}
};

}  /* namespace fds */

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_

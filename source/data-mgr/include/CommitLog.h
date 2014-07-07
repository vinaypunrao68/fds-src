/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_COMMITLOG_H_
#define SOURCE_DATA_MGR_INCLUDE_COMMITLOG_H_

#include <unistd.h>

#include <string>
#include <vector>
#include <unordered_map>

#include <fds_error.h>
#include <fds_module.h>
#include <serialize.h>
#include <blob/BlobTypes.h>
#include <DmBlobTypes.h>

namespace fds {

// commit log transaction details
struct CommitLogTx {
    BlobTxId::const_ptr txDesc;
    std::string blobName;

    bool started;
    bool commited;
    bool rolledback;

    std::vector<fds_uint64_t> entries;

    BlobObjList::ptr blobObjList;
    BlobMetaDesc::ptr blobMetaDesc;

    CommitLogTx() : txDesc(0), started(false), commited(false), rolledback(false),
            blobObjList(0), blobMetaDesc(0) {}
};

typedef std::unordered_map<BlobTxId, CommitLogTx> TxMap;

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
    explicit DmCommitLog(const std::string &modName, PersistenceType persist = IN_FILE);
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

    // update blob data (T can be BlobObjList or BlobMetaDesc)
    template<typename T>
    Error updateTx(BlobTxId::const_ptr & txDesc, boost::shared_ptr<const T> & blobData);

    // commit transaction
    Error commitTx(BlobTxId::const_ptr & txDesc);

    // rollback transaction
    Error rollbackTx(BlobTxId::const_ptr & txDesc);

    // purge transaction
    Error purgeTx(BlobTxId::const_ptr & txDesc);

    // get transaction
    Error getTx(BlobTxId::const_ptr & txDesc);

  private:
    TxMap txMap_;    // in-memory state

    PersistenceType persist_;
    boost::shared_ptr<DmCommitLogger> cmtLogger_;

    // Methods
    Error validateSubsequentTx(const BlobTxId & txId);
};


const unsigned DETAILS_BUFFER_SIZE = 4000;

const unsigned MAX_COMMIT_LOG_ENTRIES = 5000;

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
    fds_uint8_t stale;
    CommitLogEntryType type;
    fds_uint64_t id;
    fds_uint64_t txId;
    fds_uint64_t prev;

    char details[DETAILS_BUFFER_SIZE];

    DmCommitLogEntry(CommitLogEntryType type_, const BlobTxId & txDesc);

    template<typename T>
    boost::shared_ptr<T> getDetails() const {
        boost::scoped_ptr<serialize::Deserializer> d(serialize::getMemDeserializer(details));
        boost::shared_ptr<T> ret(new T());
        ret->read(d);
        return ret;
    }
};

// start transaction
struct DmCommitLogStartEntry : public DmCommitLogEntry {
    DmCommitLogStartEntry(const BlobTxId & txDesc, const std::string & blobName);
    std::string blobName() const;
};

// commit transaction
struct DmCommitLogCommitEntry : public DmCommitLogEntry {
    explicit DmCommitLogCommitEntry(const BlobTxId & txDesc) :
            DmCommitLogEntry(TX_COMMIT, txDesc) {}
};

// rollback transaction
struct DmCommitLogRollbackEntry : public DmCommitLogEntry {
    explicit DmCommitLogRollbackEntry(const BlobTxId & txDesc) :
            DmCommitLogEntry(TX_ROLLBACK, txDesc) {}
};

// purge transaction, read for compaction/ gc
struct DmCommitLogPurgeEntry : public DmCommitLogEntry {
    explicit DmCommitLogPurgeEntry(const BlobTxId & txDesc) :
            DmCommitLogEntry(TX_PURGE, txDesc) {}
};

// update blob object list
struct DmCommitLogUpdateObjListEntry : public DmCommitLogEntry {
    DmCommitLogUpdateObjListEntry(const BlobTxId & txDesc, const BlobObjList::ptr & blobObjList);
};

// update blob object meta data
struct DmCommitLogUpdateObjMetaEntry : public DmCommitLogEntry {
    DmCommitLogUpdateObjMetaEntry(const BlobTxId & txDesc, const BlobMetaDesc::ptr & meta);
};

class DmCommitLogger {
  public:
    typedef boost::shared_ptr<DmCommitLogger> ptr;
    typedef boost::shared_ptr<const DmCommitLogger> const_ptr;
};

}  /* namespace fds */

#endif  // SOURCE_DATA_MGR_INCLUDE_COMMITLOG_H_

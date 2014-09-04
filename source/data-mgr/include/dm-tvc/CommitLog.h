/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_

#include <string>
#include <vector>

#include <fds_module.h>
#include <fds_error.h>
#include <util/timeutils.h>
#include <concurrency/RwLock.h>
#include <concurrency/Mutex.h>
#include <ObjectLogger.h>
#include <blob/BlobTypes.h>
#include <DmBlobTypes.h>

/*
#include <unistd.h>
#include <string.h>
#include <set>

#include <deque>
#include <unordered_map>
#include <atomic>
#include <list>

#include <serialize.h>
#include <FdsCrypto.h>
#include <PerfTypes.h>
#include <boost/scoped_ptr.hpp>
#include <DmIoReq.h>
#include <lib/Catalog.h>
*/

namespace fds {

class DmTvcOperationJournal;

const unsigned DEFAULT_COMMIT_LOG_BUFFER_SIZE = 20 * 1024 * 1024;
const unsigned MIN_COMMIT_LOG_BUFFER_SIZE = 1 * 1024 * 1024;

const std::string COMMIT_BUFFER_FILENAME("commit.buffer");

// commit log transaction details
struct CommitLogTx : serialize::Serializable {
    typedef boost::shared_ptr<CommitLogTx> ptr;
    typedef boost::shared_ptr<const CommitLogTx> const_ptr;

    BlobTxId::const_ptr txDesc;
    std::string name;
    fds_int32_t blobMode;

    fds_uint64_t started;
    fds_uint64_t committed;     // commit issued by user, but not wriiten yet
    bool blobDelete;
    bool snapshot;

    BlobObjList::ptr blobObjList;
    MetaDataList::ptr metaDataList;

    blob_version_t blobVersion;

    CommitLogTx() : txDesc(0), blobMode(0), started(0), committed(0), blobDelete(false),
            snapshot(false), blobObjList(new BlobObjList()), metaDataList(new MetaDataList()),
            blobVersion(blob_version_invalid) {}

    virtual uint32_t write(serialize::Serializer * s) const override;
    virtual uint32_t read(serialize::Deserializer * d) override;
};

typedef std::unordered_map<BlobTxId, CommitLogTx::ptr> TxMap;

/**
 * Manages pending DM catalog updates and commits. Used
 * for 2-phase commit operations. This management
 * includes the on-disk log and the in-memory state.
 */
class DmCommitLog : public Module {
  public:
    typedef boost::shared_ptr<DmCommitLog> ptr;
    typedef boost::shared_ptr<const DmCommitLog> const_ptr;

    // ctor & dtor
    DmCommitLog(const std::string &modName, const fds_volid_t volId,
            fds::DmTvcOperationJournal & journal,
            fds_uint32_t buffersize = DEFAULT_COMMIT_LOG_BUFFER_SIZE);
    ~DmCommitLog();

    // module overrides
    int  mod_init(SysParams const *const param) override;
    void mod_startup() override;
    void mod_shutdown() override;

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

    // commit transaction (time at which commit is ACKed)
    CommitLogTx::const_ptr commitTx(BlobTxId::const_ptr & txDesc, Error & status);

    // rollback transaction
    Error rollbackTx(BlobTxId::const_ptr & txDesc);

    // snapshot
    Error snapshot(BlobTxId::const_ptr & txDesc, const std::string & name);

    // get transaction
    CommitLogTx::const_ptr getTx(BlobTxId::const_ptr & txDesc);

    // check if any transaction is pending from before the given time
    fds_bool_t isPendingTx(const fds_uint64_t tsNano = util::getTimeStampNanos());

    // process all committed entries which are not in operation journal, and clear buffer
    Error flushBuffer(std::function<Error (CommitLogTx::const_ptr)> handler,    // NOLINT
            bool safe = true);

    // reset buffer
    inline void resetBuffer() {
        SCOPEDWRITE(bufferLock_);
        buffer_->reset();
    }

    // start/ stop buffering
    inline void startBuffering() {
        buffering_ = true;
    }

    inline void stopBuffering() {
        buffering_ = false;
    }

    inline Error stopBuffering(std::function<Error (CommitLogTx::const_ptr)> handler) {  // NOLINT
        SCOPEDWRITE(bufferLock_);
        Error rc = flushBuffer(handler, false);
        buffering_ = false;
        return rc;
    }

    inline bool isBuffering() {
        return buffering_;
    }

  private:
    TxMap txMap_;    // in-memory state
    fds_rwlock lockTxMap_;

    fds_uint64_t volId_;
    fds::DmTvcOperationJournal & journal_;
    fds_uint32_t buffersize_;
    bool started_;
    // not thread-safe, two different threads can't start buffering simultaneously
    bool buffering_;

    std::string buffername_;

    boost::shared_ptr<fds::ObjectLogger> buffer_;
    fds_rwlock bufferLock_;

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

    Error snapshotInsert(BlobTxId::const_ptr & txDesc);
};
}  /* namespace fds */

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_

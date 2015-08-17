/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_

#include <mutex>
#include <string>
#include <vector>

#include <concurrency/RwLock.h>
#include <fds_module.h>
#include <fds_error.h>
#include <util/always_call.h>
#include <util/timeutils.h>
#include <ObjectLogger.h>
#include <lib/Catalog.h>
#include <blob/BlobTypes.h>
#include <DmBlobTypes.h>

namespace fds {

// commit log transaction details
struct CommitLogTx : serialize::Serializable {
    typedef boost::shared_ptr<CommitLogTx> ptr;
    typedef boost::shared_ptr<const CommitLogTx> const_ptr;

    BlobTxId::const_ptr txDesc;
    std::string name;
    fds_int32_t blobMode;

    fds_uint64_t started;
    fds_uint64_t committed;     // commit issued by user, but not written yet
    bool blobDelete;
    bool blobExpunge {false};
    bool snapshot;

    BlobObjList::ptr blobObjList;
    MetaDataList::ptr metaDataList;

    CatWriteBatch wb;

    blob_version_t blobVersion;
    fds_uint64_t nameId;
    fds_uint64_t blobSize;

    fds_uint64_t dmtVersion;

    CommitLogTx() : txDesc(0), blobMode(0), started(0), committed(0), blobDelete(false),
            snapshot(false), blobObjList(new BlobObjList()), metaDataList(new MetaDataList()),
            blobVersion(blob_version_invalid), nameId(0), blobSize(0) {}

    virtual uint32_t write(serialize::Serializer * s) const override;
    virtual uint32_t read(serialize::Deserializer * d) override;

    /*
     * with synchronizer enabled we should never see multiple updates
     * to the same tx at the same time, but synchronizer cannot be
     * enabled for block volume and can be turned off, so we to lock
     * the TX while we edit it. operations that have an exclusive
     * TxMapLock do not need to bother grabbing this mutex in additon.
     */
    std::mutex lockTx_;
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
    DmCommitLog(const std::string &modName, const fds_volid_t volId, const fds_uint32_t objSize);
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
                  fds_int32_t blobMode, fds_uint64_t dmt_version);

    // update blob data (T can be BlobObjList or MetaDataList)
    template<typename T>
    Error updateTx(BlobTxId::const_ptr & txDesc, boost::shared_ptr<const T> & blobData);

    // update blob data (T can be fpi::FDSP_BlobObjectList)
    template<typename T>
    Error updateTx(BlobTxId::const_ptr & txDesc, const T & blobData);

    // delete blob
    Error deleteBlob(BlobTxId::const_ptr & txDesc, const blob_version_t blobVersion, bool const expunge_data);

    // commit transaction (time at which commit is ACKed)
    CommitLogTx::ptr commitTx(BlobTxId::const_ptr & txDesc, Error & status);

    // rollback transaction
    Error rollbackTx(BlobTxId::const_ptr & txDesc);

    // snapshot
    Error snapshot(BlobTxId::const_ptr & txDesc, const std::string & name);

    // check if any transaction is pending from before the given time
    fds_bool_t isPendingTx(const fds_uint64_t tsNano = util::getTimeStampNanos());

    /// Block until the commit log is clear
    nullary_always getCommitLock(bool exclusive = false)
    {
        // Wait for the commit lock, then return a held lock that will
        // release when it goes out of scope to prevent blob mutations
        exclusive ? commit_lock.write_lock() : commit_lock.read_lock();
        return nullary_always([this, exclusive] { exclusive ? commit_lock.write_unlock() : commit_lock.read_unlock(); });
    }

    // take exculsive lock for map add or remove, read lock for value modification
    nullary_always getTxMapLock(bool exclusive = false)
    {
        // Wait for the lock, then return a held lock that will
        // release when it goes out of scope to prevent tx mutations
        exclusive ? txmap_lock.write_lock() : txmap_lock.read_lock();
        return nullary_always([this, exclusive] { exclusive ? txmap_lock.write_unlock() : txmap_lock.read_unlock(); });
    }

    // get active transactions
    fds_uint32_t getActiveTx() {
        auto auto_lock = getTxMapLock();
        return txMap_.size();
    }

    bool checkOutstandingTx(fds_uint64_t dmtVersion);

  private:
    TxMap txMap_;    // in-memory state
    fds_rwlock txmap_lock;
    std::unordered_map<fds_uint64_t,fds_uint64_t> dmtVerMap_;
    fds_rwlock commit_lock;

    fds_volid_t volId_;
    fds_uint32_t objSize_;
    bool started_;


    // Methods
    Error validateSubsequentTx(const BlobTxId & txId);

    void upsertBlobData(CommitLogTx & tx, boost::shared_ptr<const BlobObjList> & data) {
        std::lock_guard<std::mutex> guard(tx.lockTx_);

        if (tx.blobObjList) {
            tx.blobObjList->merge(*data);
        } else {
            tx.blobObjList.reset(new BlobObjList(*data));
        }
    }

    void upsertBlobData(CommitLogTx & tx, boost::shared_ptr<const MetaDataList> & data) {
        std::lock_guard<std::mutex> guard(tx.lockTx_);

        if (tx.metaDataList) {
            tx.metaDataList->merge(*data);
        } else {
            tx.metaDataList.reset(new MetaDataList(*data));
        }
    }

    void upsertBlobData(CommitLogTx & tx, const fpi::FDSP_BlobObjectList & data);

    Error snapshotInsert(BlobTxId::const_ptr & txDesc);
};
}  /* namespace fds */

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_COMMITLOG_H_

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_

#include <string>
#include <functional>
#include <fds_error.h>
#include <fds_module.h>
#include <fds_config.hpp>
#include <DmBlobTypes.h>
#include <dm-tvc/CommitLog.h>
#include <dm-tvc/OperationJournal.h>
#include <dm-vol-cat/DmVolumeCatalog.h>
#include <util/Log.h>
#include <concurrency/SynchronizedTaskExecutor.hpp>

namespace fds {

/**
 * The time volume catalog manages the update history of a volume
 * and current pending updates to a volume. The TVC allows updates
 * to happen in a transactional context and provides interfaces
 * for historical information to be queried.
 */
class DmTimeVolCatalog : public Module, boost::noncopyable {
  private:
    /* Lock around commit log */
    fds_spinlock commitLogLock_;

    /**
     * Log the manages recent and currently active
     * blob transactions
     */
    std::unordered_map<fds_volid_t, DmCommitLog::ptr> commitLogs_;

    /* Lock around op journal */
    fds_spinlock opJournalLock_;

    /**
     * Per volume operation journal, keeps long term log of
     * blob write operations
     */
    std::unordered_map<fds_volid_t, DmTvcOperationJournal::ptr> opJournals_;

    /**
     * For executing certain blob operatins (commit, delete) in a
     * synchronized manner
     */
    SynchronizedTaskExecutor<std::string> opSynchronizer_;

    // TODO(Andrew): Add a history log eventually...

    /**
     * Internal volume catalog pointer. The actual catalog is owned by
     * the TVC and only a query interface is exposed
     */
    DmVolumeCatalog::ptr volcat;

    /**
     * Notifies TVC of transactions that have been persisted
     * in the volume catalog. The notification allows the TVC
     * to know which log entries may be pruned.
     * @param[in] syncdTxList  List of sync'd transaction IDs
     *
     * @return none
     */
    void notifyVolCatalogSync(BlobTxList::const_ptr sycndTxList);

    // as the configuration will not be refreshed frequently, we can read it without lock
    FdsConfigAccessor config_helper_;

  public:
    /**
     * Constructs the TVC object but does not init
     */
    DmTimeVolCatalog(const std::string &modName, fds_threadpool &tp);
    ~DmTimeVolCatalog();

    typedef boost::shared_ptr<DmTimeVolCatalog> ptr;
    typedef boost::shared_ptr<const DmTimeVolCatalog> const_ptr;
    typedef std::function<void (const Error &,
                                blob_version_t,
                                const BlobObjList::const_ptr&,
                                const MetaDataList::const_ptr&)> CommitCb;
    typedef std::function<void (const Error &)> FwdCommitCb;

    /// Allow sync related interface to volume catalog
    friend class DmVolumeCatalog;

    /**
     * Notification about new volume managed by this DM.
     * This could be either completely new volume or volume
     * that was previously managed by other DMs. When this method
     * returns, TVC is not ready to accept requests for the volume
     * untill activateVolume() is called.
     * @param[in] voldesc descriptor of new volume
     * @return ERR_OK if TVC and Volume Catalog is ready
     * for volume activate
     */
    Error addVolume(const VolumeDesc& voldesc);

    /**
     * Create copy of the volmume  for snapshot/clone
     */
    Error copyVolume(VolumeDesc & voldesc);

    /**
     * Prepare TVC and Volume Catalog to accept requests for
     * this volume
     * @param[in] volId volume ID
     */
    Error activateVolume(fds_volid_t volId);

    /**
     * Marks volume as deleted if the volume does not contain any valid blobs.
     * A valid blob is a non-deleted blob version. If volume marked as deleted,
     * updates to the volume catalog will be rejected.
     * @return ERR_OK if volume is empty and marked as deleted;
     *  ERR_VOL_NOT_EMPTY if volume contains at least one valid blob
     */
    Error markVolumeDeleted(fds_volid_t volId);

    /**
     * Removes volume marked as deleted
     * @return ERR_OK if volume is deleted; ERR_NOT_READY if volume not marked
     * as delted
     */
    Error deleteEmptyVolume(fds_volid_t volId);

    /**
     * Starts a new transaction for blob
     * @param[in] volId volume ID
     * @param[in] blobName Name of blob
     * @param[in] blobMode  Blob mode
     * @param[in] txDesc   Transaction ID
     *
     * @return ERR_OK if the transaction was successfully
     * started
     */
    Error startBlobTx(fds_volid_t volId,
                      const std::string &blobName,
                      fds_int32_t blobMode,
                      BlobTxId::const_ptr txDesc);
    /**
     * Applies a new offset update to an existing transaction
     * @param[in] volId volume ID
     * @param[in] txDesc  Transaction ID
     * @param[in] objList List of blob offsets being updated
     *
     * @return ERR_OK if the update was successfully applied
     * to the transaction
     */
    Error updateBlobTx(fds_volid_t volId,
                       BlobTxId::const_ptr txDesc,
                       const fpi::FDSP_BlobObjectList &objList);

    /**
     * Applies a new medatadata update to an
     * existing transaction
     * @param[in] volId volume ID
     * @param[in] txDesc  Transaction ID
     * @param[in] objList List of blob offsets being updated
     *
     * @return ERR_OK if the update was successfully applied
     * to the transaction
     */
    Error updateBlobTx(fds_volid_t volId,
                       BlobTxId::const_ptr txDesc,
                       const fpi::FDSP_MetaDataList &metaList);


    /**
     * Deletes blob in a transaction
     * If the blob version is invalid, deletes the most recent blob version.
     * @param[in] volId Volume ID
     * @param[in] txDesc Transaction ID
     * @param[in] blob_version blob version of blob_version_invalid to
     * delete the most recent blob version
     *
     * @return ERR_OK on success
     */
    Error deleteBlob(fds_volid_t volId, BlobTxId::const_ptr txDesc,
                     blob_version_t blob_version);

    /**
     * Commits the updates associated with an existing transaction
     * @param[in] volId volume ID
     * @param[in] blobName Name of blob
     * @param[in] txDesc Transaction ID
     * @param[in] commitCb commit callback
     *
     * @return ERR_OK if the commit was successfully applied
     */
    Error commitBlobTx(fds_volid_t volId,
                       const std::string &blobName,
                       BlobTxId::const_ptr txDesc,
                       const DmTimeVolCatalog::CommitCb &commitCb);

    /**
     * Aborts an existing transaction. All pending updates
     * associated with the transaction are discarded.
     * @param[in] volId volumeID
     * @param[in] txDesc Transaction ID
     *
     * @return ERR_OK if the abort was successfully applied
     */
    Error abortBlobTx(fds_volid_t volId,
                      BlobTxId::const_ptr txDesc);

    /**
     * Updates forwarded committed blob from remote DM; This is
     * an update that was already committed on DM that is migrating
     * volume catalog to this DM, so the update is serialized and
     * directly committed to volume catalog without going through
     * commit log.
     * @param[in] volId volume ID
     * @param[in] blobName name of the blob
     * @param[in] blobVersion version of the blob or blob_version_deleted
     * if this is a forwarded committed blob delete
     * @param[in] objList list of offset to object mappings or NULL
     * @param[in] metaList list of meta k-v pairs if any, or NULL
     */
    Error updateFwdCommittedBlob(fds_volid_t volId,
                                 const std::string &blobName,
                                 blob_version_t blobVersion,
                                 const fpi::FDSP_BlobObjectList &objList,
                                 const fpi::FDSP_MetaDataList &metaList,
                                 const DmTimeVolCatalog::FwdCommitCb &fwdCommitCb);

    /**
     * Returns true if there are any pending transactions that started
     * before given time
     * @param[in] timeNano time in nanoseconds
     */
    fds_bool_t isPendingTx(fds_volid_t volId, fds_uint64_t timeNano);

    void commitBlobTxWork(fds_volid_t volid,
                          DmCommitLog::ptr &commitLog,
                          BlobTxId::const_ptr txDesc,
                          const DmTimeVolCatalog::CommitCb &cb);

    Error doCommitBlob(fds_volid_t volid, blob_version_t & blob_version,
            CommitLogTx::const_ptr commit_data);

    void updateFwdBlobWork(fds_volid_t volId,
                           const std::string &blobName,
                           blob_version_t blobVersion,
                           const fpi::FDSP_BlobObjectList &objList,
                           const fpi::FDSP_MetaDataList &metaList,
                           const DmTimeVolCatalog::FwdCommitCb &fwdCommitCb);

    /**
     * Returns query interface to volume catalog. Provides
     * abstraction of volume catalog objects and management
     * from caller.
     */
    inline VolumeCatalogQueryIface::ptr queryIface() {
        return volcat;
    }

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_

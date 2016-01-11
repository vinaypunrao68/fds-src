/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <fds_error.h>
#include <fds_module.h>
#include <fds_config.hpp>
#include <DmBlobTypes.h>
#include <dm-tvc/CommitLog.h>
#include <dm-vol-cat/DmVolumeCatalog.h>
#include <util/Log.h>
#include <util/timeutils.h>
#include <concurrency/SynchronizedTaskExecutor.hpp>

namespace fds {

struct DmVolumeAccessTable;

/**
 * The time volume catalog manages the update history of a volume
 * and current pending updates to a volume. The TVC allows updates
 * to happen in a transactional context and provides interfaces
 * for historical information to be queried.
 */
class DmTimeVolCatalog : public HasModuleProvider,
                         Module, boost::noncopyable {
  private:
    DataMgr& dataManager_;

    /* Lock around commit log */
    fds_mutex commitLogLock_;

    enum TimeVolumeCatalogState {
        /**
         * Time Volume Catalog is in initializing state before
         * it validates its persistent layer (mount points) -- e.g.
         * whether disk exists, there is enough capacity, etc.
         */
        TVC_INIT           = 0,
        /**
         * Ready to service IO requests
         */
        TVC_READY          = 1,
        /**
         * Something bad happened, e.g. mount point unreachable,
         * no disk space left, so TVC is currently un-usable
         */
        TVC_UNAVAILABLE    = 2,
        TVC_STATE_MAX
    };

    /// Currentl state of TVC
    std::atomic<TimeVolumeCatalogState> currentState;

    /**
     * Log the manages recent and currently active
     * blob transactions
     */
    std::unordered_map<fds_volid_t, DmCommitLog::ptr> commitLogs_;

    /** Mutual exclusion for AccessTable operations */
    std::mutex accessTableLock_;

    /**
     * Volume access map. Each volume has an access policy which controls
     * the exclusivity of AM attachments.
     */
    std::unordered_map<fds_volid_t, std::unique_ptr<DmVolumeAccessTable>> accessTable_;
    std::chrono::duration<fds_uint32_t> vol_tok_lease_time {60};

    /**
     * For executing certain blob operations (commit, delete) in a
     * synchronized manner
     */
    SynchronizedTaskExecutor<fds_uint64_t> opSynchronizer_;

    // TODO(Andrew): Add a history log eventually...

    /**
     * Internal volume catalog pointer. The actual catalog is owned by
     * the TVC and only a query interface is exposed
     */
    DmVolumeCatalog::ptr volcat;

    // as the configuration will not be refreshed frequently, we can read it without lock
    FdsConfigAccessor config_helper_;

    // threadpool
    fds_threadpool & tp_;

    /**
     * Notifies TVC of transactions that have been persisted
     * in the volume catalog. The notification allows the TVC
     * to know which log entries may be pruned.
     * @param[in] syncdTxList  List of sync'd transaction IDs
     *
     * @return none
     */
    void notifyVolCatalogSync(BlobTxList::const_ptr sycndTxList);

  protected:
    void createCommitLog(const VolumeDesc& voldesc);

  public:
    /**
     * Constructs the TVC object but does not init
     */
    DmTimeVolCatalog(CommonModuleProviderIf *modProvider,
                     const std::string &modName,
                     fds_threadpool &tp,
                     DataMgr& dataManager);
    ~DmTimeVolCatalog();

    typedef boost::shared_ptr<DmTimeVolCatalog> ptr;
    typedef boost::shared_ptr<const DmTimeVolCatalog> const_ptr;
    typedef std::function<void (const Error &,
                                blob_version_t,
                                const BlobObjList::const_ptr&,
                                const MetaDataList::const_ptr&,
                                const fds_uint64_t)> CommitCb;

    /// Allow sync related interface to volume catalog
    friend class DmVolumeCatalog;

    /**
     * Sets TVC to unavailable state -- all write IO
     * will be rejected
     */
    void setUnavailable();
    fds_bool_t isUnavailable();

    /**
     * Notification about new volume managed by this DM.
     * This could be either completely new volume or volume
     * that was previously managed by other DMs. When this method
     * returns, TVC is not ready to accept requests for the volume
     * until activateVolume() is called.
     * @param[in] voldesc descriptor of new volume
     * @return ERR_OK if TVC and Volume Catalog is ready
     * for volume activate
     */
    Error addVolume(const VolumeDesc& voldesc);

    /**
     * Attempt to "open" this volume for access
     */
    Error openVolume(fds_volid_t const volId,
                     fpi::SvcUuid const& client_uuid,
                     fds_int64_t& token,
                     fpi::VolumeAccessMode const& mode,
                     sequence_id_t& sequence_id,
                     int32_t &version);

    /**
     * Attempt to "close" this volume from a previous open
     */
    Error closeVolume(fds_volid_t const volId, fds_int64_t const token);

    /**
     * Create copy of the volume for snapshot/clone
     */
    Error copyVolume(VolumeDesc & voldesc,  fds_volid_t origSrcVolume = invalid_vol_id);

    /**
     * Increment object reference counts for all objects referred by source
     * volume. Sends message to SM with list of object IDs.
     */
    void incrObjRefCount(fds_volid_t srcVolId, fds_volid_t destVolId, fds_token_id token,
            boost::shared_ptr<std::vector<fpi::FDS_ObjectIdType> > objIds);

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
     * as deleted
     */
    Error deleteEmptyVolume(fds_volid_t volId);

    /**
     * Sets the key-value metadata pairs for the volume. Any keys that already
     * existed are overwritten and previously set keys are left unchanged.
     * @param[in] volId The ID of the volume's catalog to update
     * @param[in] metadataList A list of metadata key value pairs to set.
     * @return ERR_OK on success, ERR_VOL_NOT_FOUND if volume is not known
     * to volume catalog.
     */
    Error setVolumeMetadata(fds_volid_t volId,
                            const fpi::FDSP_MetaDataList &metadataList,
                            const sequence_id_t seq_id);

    /**
     * Starts a new transaction for blob
     * @param[in] volId volume ID
     * @param[in] blobName Name of blob
     * @param[in] blobMode  Blob mode
     * @param[in] txDesc   Transaction ID
     * @param[in] dmtVersion DMT version
     *
     * @return ERR_OK if the transaction was successfully
     * started
     */
    Error startBlobTx(fds_volid_t volId,
                      const std::string &blobName,
                      fds_int32_t blobMode,
                      BlobTxId::const_ptr txDesc,
                      fds_uint64_t dmtVersion);
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
     * Applies a new metadata update to an
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
    Error deleteBlob(fds_volid_t const volId,
                     BlobTxId::const_ptr txDesc,
                     blob_version_t const blob_version,
                     bool const expunge_data = true);

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
                       sequence_id_t seq_id,
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
                                 const sequence_id_t seq_id);

    /**
     * Returns true if there are any pending transactions that started
     * before given time
     * @param[in] timeNano time in nanoseconds
     */
    fds_bool_t isPendingTx(fds_volid_t volId, fds_uint64_t timeNano);

    void commitBlobTxWork(fds_volid_t volid,
                          const std::string &blobName,
                          DmCommitLog::ptr &commitLog,
                          BlobTxId::const_ptr txDesc,
                          sequence_id_t seq_id,
                          const DmTimeVolCatalog::CommitCb &cb);

    Error doCommitBlob(fds_volid_t volid, blob_version_t & blob_version,
            sequence_id_t seq_id, CommitLogTx::ptr commit_data);

    Error getCommitlog(fds_volid_t volId,  DmCommitLog::ptr &commitLog);

    /**
     * insert blob descriptors into catalog during static migration, bypassing the
     * commit log.
     * NOTE: do NOT use for any data path operation.
     */
    //protected:
    /* XXX: public only for unit tests because I can't get
       gtest friend classing to work */
    Error migrateDescriptor(fds_volid_t volId,
                            const std::string& blobName,
                            const std::string& blobData);
    /*friend class DmMigrationExecutor;
    // for gtest
    friend class SeqIdTest_MigrateVolDesc_Test;
    friend class SeqIdTest_MigrateBlobDelete_Test;
    friend class SeqIdTest_MigrateBlobPutNew_Test;
    public:*/

    /**
     * Returns query interface to volume catalog. Provides
     * abstraction of volume catalog objects and management
     * from caller.
     */
    inline VolumeCatalogQueryIface::ptr queryIface() {
        return volcat;
    }

    /**
     * Method to get % of utilized space for the DM's partition
     */
    float_t getUsedCapacityAsPct();

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_

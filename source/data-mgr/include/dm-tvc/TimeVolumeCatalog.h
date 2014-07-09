/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_

#include <string>
#include <fds_error.h>
#include <fds_module.h>
#include <DmBlobTypes.h>
#include <CommitLog.h>
#include <dm-vol-cat/DmVolumeCatalog.h>
#include <util/Log.h>

namespace fds {

/**
 * The time volume catalog manages the update history of a volume
 * and current pending updates to a volume. The TVC allows updates
 * to happen in a transactional context and provides interfaces
 * for historical information to be queried.
 */
class DmTimeVolCatalog : public Module, boost::noncopyable {
  private:
    /**
     * Log the manages recent and currently active
     * blob transactions
     */
    DmCommitLog::ptr commitLog;

    // TODO(Andrew): Add a history log eventually...

    /**
     * Notifies TVC of transactions that have been persisted
     * in the volume catalog. The notification allows the TVC
     * to know which log entries may be pruned.
     * @param[in] syncdTxList  List of sync'd transaction IDs
     *
     * @return none
     */
    void notifyVolCatalogSync(BlobTxList::const_ptr sycndTxList);

  public:
    /**
     * Constructs the TVC object but does not init
     */
    explicit DmTimeVolCatalog(const std::string &modName);
    ~DmTimeVolCatalog();

    typedef boost::shared_ptr<DmTimeVolCatalog> ptr;
    typedef boost::shared_ptr<const DmTimeVolCatalog> const_ptr;

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
     * Prepare TVC and Volume Catalog to accept requests for
     * this volume
     * @param[in] volId volume ID
     */
    Error activateVolume(fds_volid_t volId);

    /**
     * Starts a new transaction for blob
     * @param[in] volId volume ID
     * @param[in] blobName Name of blob
     * @param[in] txDesc   Transaction ID
     *
     * @return ERR_OK if the transaction was successfully
     * started
     */
    Error startBlobTx(fds_volid_t volId,
                      const std::string &blobName,
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
                       const BlobTxId::const_ptr txDesc,
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
                       const BlobTxId::const_ptr txDesc,
                       const fpi::FDSP_MetaDataList &metaList);


    /**
     * Commits the updates associated with an existing transaction
     * @param[in] volId volume ID
     * @param[in] txDesc Transaction ID
     *
     * @return ERR_OK if the commit was successfully applied
     */
    Error commitBlobTx(fds_volid_t volId,
                       const BlobTxId::const_ptr txDesc);

    /**
     * Aborts an existing transaction. All pending updates
     * associated with the transaction are discarded.
     * @param[in] volId volumeID
     * @param[in] txDesc Transaction ID
     *
     * @return ERR_OK if the abort was successfully applied
     */
    Error abortBlobTx(fds_volid_t volId,
                      const BlobTxId::const_ptr txDesc);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

extern DmTimeVolCatalog gl_DmTvcMod;

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_

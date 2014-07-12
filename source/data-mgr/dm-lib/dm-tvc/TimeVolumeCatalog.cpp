/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <boost/make_shared.hpp>
#include <dm-tvc/TimeVolumeCatalog.h>

#define COMMITLOG_GET(_volId_, _commitLog_) \
    do { \
        fds_scoped_spinlock l(commitLogLock_); \
        try { \
            _commitLog_ = commitLogs_.at(_volId_); \
        } catch(const std::out_of_range &oor) { \
            return ERR_VOL_NOT_FOUND; \
        } \
    } while (false)

namespace fds {

void
DmTimeVolCatalog::notifyVolCatalogSync(BlobTxList::const_ptr sycndTxList) {
}

DmTimeVolCatalog::DmTimeVolCatalog(const std::string &name, fds_threadpool &tp)
        : Module(name.c_str()),
        opSynchronizer_(tp)
{
    volcat = new DmVolumeCatalog("DM Volume Catalog");
    static Module* om_mods[] = {
        volcat,
        NULL
    };
}

DmTimeVolCatalog::~DmTimeVolCatalog() {
    delete volcat;
}

/**
 * Module initialization
 */
int
DmTimeVolCatalog::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
DmTimeVolCatalog::mod_startup() {
}

/**
 * Module shutdown
 */
void
DmTimeVolCatalog::mod_shutdown() {
}

Error
DmTimeVolCatalog::addVolume(const VolumeDesc& voldesc) {
    LOGDEBUG << "Will prepare commit log for new volume "
             << std::hex << voldesc.volUUID << std::dec;

    fds_scoped_spinlock l(commitLogLock_);
    if (commitLogs_.find(voldesc.volUUID) != commitLogs_.end()) {
        return ERR_VOL_DUPLICATE;
    }

    std::ostringstream oss;
    oss << "Volume_" << voldesc.volUUID;
    /* NOTE: Here the lock can be expensive.  We may want to provide an init() api
     * on DmCommitLog so that initialization can happen outside the lock
     */
    commitLogs_[voldesc.volUUID] = boost::make_shared<DmCommitLog>("DM", oss.str());
    commitLogs_[voldesc.volUUID]->mod_init(mod_params);
    commitLogs_[voldesc.volUUID]->mod_startup();

    return volcat->addCatalog(voldesc);
}

Error
DmTimeVolCatalog::activateVolume(fds_volid_t volId) {
    LOGDEBUG << "Will activate commit log for volume "
             << std::hex << volId << std::dec;
    // TODO(Anna): revisit if we need to do anything with commit log
    // when integrating meta sync (multi-dm)
    return volcat->activateCatalog(volId);
}

Error
DmTimeVolCatalog::startBlobTx(fds_volid_t volId,
                              const std::string &blobName,
                              BlobTxId::const_ptr txDesc) {
    LOGDEBUG << "Starting transaction " << *txDesc
             << " for blob " << blobName << " volume "
             << std::hex << volId << std::dec;
    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(volId, commitLog);
    return commitLog->startTx(txDesc, blobName);
}

Error
DmTimeVolCatalog::updateBlobTx(fds_volid_t volId,
                               BlobTxId::const_ptr txDesc,
                               const fpi::FDSP_BlobObjectList &objList) {
    LOGDEBUG << "Updating offsets for transaction " << *txDesc
             << " volume " << std::hex << volId << std::dec;
    auto boList = boost::make_shared<const BlobObjList>(objList);

    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(volId, commitLog);
    return commitLog->updateTx(txDesc, boList);
}

Error
DmTimeVolCatalog::updateBlobTx(fds_volid_t volId,
                               BlobTxId::const_ptr txDesc,
                               const fpi::FDSP_MetaDataList &metaList) {
    LOGDEBUG << "Updating metadata for transaction " << *txDesc
             << " volume " << std::hex << volId << std::dec;

    auto mdList = boost::make_shared<const MetaDataList>(metaList);
    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(volId, commitLog);
    return commitLog->updateTx(txDesc, mdList);
}

Error
DmTimeVolCatalog::commitBlobTx(fds_volid_t volId,
                               const std::string &blobName,
                               BlobTxId::const_ptr txDesc,
                               const DmTimeVolCatalog::CommitCb &cb) {
    LOGDEBUG << "Will commit transaction " << *txDesc << " for volume "
             << std::hex << volId << std::dec;
    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(volId, commitLog);
    opSynchronizer_.schedule(blobName,
                             std::bind(&DmTimeVolCatalog::commitBlobTxWork,
                                       this, volId, commitLog, txDesc, cb));
    return ERR_OK;
}

void
DmTimeVolCatalog::commitBlobTxWork(fds_volid_t volid,
                                   DmCommitLog::ptr &commitLog,
                                   BlobTxId::const_ptr txDesc,
                                   const DmTimeVolCatalog::CommitCb &cb) {
    Error e;
    LOGDEBUG << "Committing transaction " << *txDesc << " for volume "
             << std::hex << volid << std::dec;
    CommitLogTx::const_ptr commit_data = commitLog->commitTx(txDesc, e);
    if (e.ok()) {
        if (commit_data->blobObjList &&
            (commit_data->blobObjList->size() > 0)) {
            e = volcat->putBlobMeta(volid, commit_data->blobName,
                                    commit_data->metaDataList, txDesc);
        } else {
            e = volcat->putBlob(volid, commit_data->blobName, commit_data->metaDataList,
                                commit_data->blobObjList, txDesc);
        }
    }
    cb(e);
}

Error
DmTimeVolCatalog::abortBlobTx(fds_volid_t volId,
                              BlobTxId::const_ptr txDesc) {
    return ERR_OK;
}

// TODO(Rao):
// 1. Probably shouldn't be a global.
// 2. Get the threadpool from DM
static fds_threadpool s_threadpool;
DmTimeVolCatalog gl_DmTvcMod("Global DM TVC", s_threadpool);

}  // namespace fds

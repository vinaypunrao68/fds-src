/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <set>
#include <map>
#include <limits>
#include <DataMgr.h>

#include <boost/make_shared.hpp>
#include <dm-tvc/TimeVolumeCatalog.h>
#include <fds_process.h>
#include <ObjectLogger.h>

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
        : Module(name.c_str()), opSynchronizer_(tp),
        config_helper_(g_fdsprocess->get_conf_helper()), tp_(tp)
{
    volcat = DmVolumeCatalog::ptr(new DmVolumeCatalog("DM Volume Catalog"));
    // TODO(Andrew): The module vector should be able to take smart pointers.
    // To get around this for now, we're extracting the raw pointer and
    // expecting that any reference to are done once this returns...
    static Module* tvcMods[] = {
        static_cast<Module *>(volcat.get()),
        NULL
    };
}

DmTimeVolCatalog::~DmTimeVolCatalog() {
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
    Error rc = ERR_OK;
    {
        fds_scoped_spinlock l(commitLogLock_);
        /* NOTE: Here the lock can be expensive.  We may want to provide an init() api
         * on DmCommitLog so that initialization can happen outside the lock
         */
        commitLogs_[voldesc.volUUID] = boost::make_shared<DmCommitLog>("DM", voldesc.volUUID);
        commitLogs_[voldesc.volUUID]->mod_init(mod_params);
        commitLogs_[voldesc.volUUID]->mod_startup();

        rc = volcat->addCatalog(voldesc);
    }

    return rc;
}

Error
DmTimeVolCatalog::copyVolume(VolumeDesc & voldesc) {
    Error rc(ERR_OK);
    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(voldesc.srcVolumeId, commitLog);
    fds_assert(commitLog);

    LOGDEBUG << "Creating a snapshot '" << voldesc.name << "' of a volume '"
            << voldesc.volUUID << "'"  << "srcVolume: "<< voldesc.srcVolumeId;
    if (voldesc.isSnapshot()) {
        // TODO(umesh): remove this, underlying logic is changed.
        BlobTxId::const_ptr txId(new BlobTxId(voldesc.srcVolumeId));
        rc = commitLog->snapshot(txId, voldesc.name);
        if (!rc.ok()) {
            GLOGERROR << "Failed to write entry into commit log for snapshot '" << std::hex <<
                   voldesc.volUUID << std::dec << "', volume '" << std::hex << voldesc.srcVolumeId;
            return rc;
        }
    }

    // Create snapshot of volume catalog
    rc = volcat->copyVolume(voldesc);
    if (!rc.ok()) {
        GLOGERROR << "Failed to copy catalog for snapshot '" << std::hex <<
                voldesc.volUUID << std::dec << "', volume '" << std::hex <<
                voldesc.srcVolumeId;
        return rc;
    }

    if (dataMgr->amIPrimary(voldesc.srcVolumeId)) {
        // Increment object references
        std::set<ObjectID> objIds;
        rc = volcat->getVolumeObjects(voldesc.srcVolumeId, objIds);
        if (!rc.ok()) {
            GLOGCRITICAL << "Failed to get object ids for volume '" << std::hex <<
                    voldesc.srcVolumeId << std::dec << "'";
            return rc;
        }

        OMgrClient * omClient = dataMgr->omClient;
        fds_verify(omClient);

        std::map<fds_token_id, boost::shared_ptr<std::vector<fpi::FDS_ObjectIdType> > >
                tokenOidMap;
        for (auto oid : objIds) {
            const DLT * dlt = omClient->getCurrentDLT();
            fds_verify(dlt);

            fds_token_id token = dlt->getToken(oid);
            if (!tokenOidMap[token].get()) {
                tokenOidMap[token].reset(new std::vector<fpi::FDS_ObjectIdType>());
            }

            fpi::FDS_ObjectIdType tmpId;
            fds::assign(tmpId, oid);
            tokenOidMap[dlt->getToken(oid)]->push_back(tmpId);
        }

        for (auto it : tokenOidMap) {
            tp_.schedule(&DmTimeVolCatalog::incrObjRefCount, this, voldesc.srcVolumeId,
                    voldesc.volUUID, it.first, it.second);
        }
    }

    return rc;
}

void
DmTimeVolCatalog::incrObjRefCount(fds_volid_t srcVolId, fds_volid_t destVolId,
        fds_token_id token, boost::shared_ptr<std::vector<fpi::FDS_ObjectIdType> > objIds) {
    // TODO(umesh): implement this
}

Error
DmTimeVolCatalog::activateVolume(fds_volid_t volId) {
    LOGDEBUG << "Will activate commit log for volume "
             << std::hex << volId << std::dec;
    return volcat->activateCatalog(volId);
}

Error
DmTimeVolCatalog::markVolumeDeleted(fds_volid_t volId) {
    if (isPendingTx(volId, 0)) return ERR_VOL_NOT_EMPTY;
    Error err = volcat->markVolumeDeleted(volId);
    if (err.ok()) {
        // TODO(Anna) @Umesh we should mark commit log as
        // deleted and reject all tx to this commit log
        // only mark log as deleted if no pending tx, otherwise
        // return error
        LOGDEBUG << "Marked volume as deleted, vol "
                 << std::hex << volId << std::dec;
    }
    return err;
}

Error
DmTimeVolCatalog::deleteEmptyVolume(fds_volid_t volId) {
    Error err = volcat->deleteEmptyCatalog(volId);
    if (err.ok()) {
        fds_scoped_spinlock l(commitLogLock_);
        if (commitLogs_.count(volId) > 0) {
            // found commit log
            commitLogs_.erase(volId);
        }
    }
    return err;
}

Error
DmTimeVolCatalog::startBlobTx(fds_volid_t volId,
                              const std::string &blobName,
                              const fds_int32_t blobMode,
                              BlobTxId::const_ptr txDesc) {
    LOGDEBUG << "Starting transaction " << *txDesc << " for blob " << blobName <<
            " volume " << std::hex << volId << std::dec << " blob mode " << blobMode;
    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(volId, commitLog);
    return commitLog->startTx(txDesc, blobName, blobMode);
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
DmTimeVolCatalog::deleteBlob(fds_volid_t volId,
                             BlobTxId::const_ptr txDesc,
                             blob_version_t blob_version) {
    LOGDEBUG << "Deleting Blob for transaction " << *txDesc << ", volume " <<
            std::hex << volId << std::dec << " version " << blob_version;

    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(volId, commitLog);
    return commitLog->deleteBlob(txDesc, blob_version);
}

Error
DmTimeVolCatalog::commitBlobTx(fds_volid_t volId,
                               const std::string &blobName,
                               BlobTxId::const_ptr txDesc,
                               const DmTimeVolCatalog::CommitCb &cb) {
    LOGDEBUG << "Will commit transaction " << *txDesc << " for volume "
             << std::hex << volId << std::dec << " blob " << blobName;
    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(volId, commitLog);
    opSynchronizer_.schedule(blobName,
                             std::bind(&DmTimeVolCatalog::commitBlobTxWork,
                                       this, volId, commitLog, txDesc, cb));
    return ERR_OK;
}

Error
DmTimeVolCatalog::updateFwdCommittedBlob(fds_volid_t volId,
                                         const std::string &blobName,
                                         blob_version_t blobVersion,
                                         const fpi::FDSP_BlobObjectList &objList,
                                         const fpi::FDSP_MetaDataList &metaList,
                                         const DmTimeVolCatalog::FwdCommitCb &fwdCommitCb) {
    LOGDEBUG << "Will apply committed blob update from another DM for volume "
             << std::hex << volId << std::dec << " blob " << blobName;

    // we don't go through commit log, but we need to serialized fwd updates
    opSynchronizer_.schedule(blobName,
                             std::bind(&DmTimeVolCatalog::updateFwdBlobWork,
                                       this, volId, blobName, blobVersion,
                                       objList, metaList, fwdCommitCb));

    return ERR_OK;
}

void
DmTimeVolCatalog::commitBlobTxWork(fds_volid_t volid,
                                   DmCommitLog::ptr &commitLog,
                                   BlobTxId::const_ptr txDesc,
                                   const DmTimeVolCatalog::CommitCb &cb) {
    Error e;
    blob_version_t blob_version = blob_version_invalid;
    LOGDEBUG << "Committing transaction " << *txDesc << " for volume "
             << std::hex << volid << std::dec;
    CommitLogTx::const_ptr commit_data = commitLog->commitTx(txDesc, e);
    if (e.ok()) {
        e = doCommitBlob(volid, blob_version, commit_data);
    }
    cb(e, blob_version, commit_data->blobObjList, commit_data->metaDataList);
}

Error
DmTimeVolCatalog::doCommitBlob(fds_volid_t volid, blob_version_t & blob_version,
        CommitLogTx::const_ptr commit_data) {
    Error e;
    if (commit_data->blobDelete) {
        e = volcat->deleteBlob(volid, commit_data->name, commit_data->blobVersion);
        blob_version = commit_data->blobVersion;
    } else {
        if (commit_data->blobObjList && !commit_data->blobObjList->empty()) {
            if (commit_data->blobMode & blob::TRUNCATE) {
                commit_data->blobObjList->setEndOfBlob();
            }
            e = volcat->putBlob(volid, commit_data->name, commit_data->metaDataList,
                    commit_data->blobObjList, commit_data->txDesc);
        }
        if (commit_data->metaDataList && !commit_data->metaDataList->empty()) {
            e = volcat->putBlobMeta(volid, commit_data->name,
                    commit_data->metaDataList, commit_data->txDesc);
        }
    }

    return e;
}

void DmTimeVolCatalog::updateFwdBlobWork(fds_volid_t volid,
                                         const std::string &blobName,
                                         blob_version_t blobVersion,
                                         const fpi::FDSP_BlobObjectList &objList,
                                         const fpi::FDSP_MetaDataList &metaList,
                                         const DmTimeVolCatalog::FwdCommitCb &fwdCommitCb) {
    Error err(ERR_OK);
    // TODO(xxx): use blob mode to tell if that's a deletion
    if (blobVersion == blob_version_deleted) {
        LOGDEBUG << "Applying committed forwarded delete for blob " << blobName
                 << " volume " << std::hex << volid << std::dec;
        fds_panic("not implemented!");
        // err = volcat->deleteBlob(volid, blobName, blobVersion);
    } else {
        LOGDEBUG << "Applying committed forwarded update for blob " << blobName
                 << " volume " << std::hex << volid << std::dec;

        if (objList.size() > 0) {
            BlobObjList::ptr olist(new(std::nothrow) BlobObjList(objList));
            // TODO(anna) pass truncate op, for now always truncate
            olist->setEndOfBlob();

            if (metaList.size() > 0) {
                MetaDataList::ptr mlist(new(std::nothrow) MetaDataList(metaList));
                err = volcat->putBlob(volid, blobName, mlist, olist, BlobTxId::ptr());
            } else {
                err = volcat->putBlob(volid, blobName, MetaDataList::ptr(),
                                      olist, BlobTxId::ptr());
            }
        } else {
            fds_verify(metaList.size() > 0);
            MetaDataList::ptr mlist(new(std::nothrow) MetaDataList(metaList));
            err = volcat->putBlobMeta(volid, blobName, mlist, BlobTxId::ptr());
        }
    }

    fwdCommitCb(err);
}

Error
DmTimeVolCatalog::abortBlobTx(fds_volid_t volId,
                              BlobTxId::const_ptr txDesc) {
    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(volId, commitLog);
    return commitLog->rollbackTx(txDesc);
}

fds_bool_t
DmTimeVolCatalog::isPendingTx(fds_volid_t volId, fds_uint64_t timeNano) {
    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(volId, commitLog);
    return commitLog->isPendingTx(timeNano);
}

Error
DmTimeVolCatalog::getCommitlog(fds_volid_t volId,  DmCommitLog::ptr &commitLog) {
    Error rc(ERR_OK);
    COMMITLOG_GET(volId, commitLog);
    return rc;
}

}  // namespace fds

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <sys/types.h>
#include <sys/inotify.h>
#include <dirent.h>

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
#include <leveldb/copy_env.h>
#include <leveldb/cat_journal.h>

#define COMMITLOG_GET(_volId_, _commitLog_) \
    do { \
        fds_scoped_spinlock l(commitLogLock_); \
        try { \
            _commitLog_ = commitLogs_.at(_volId_); \
        } catch(const std::out_of_range &oor) { \
            return ERR_VOL_NOT_FOUND; \
        } \
    } while (false)

static const fds_uint32_t POLL_WAIT_TIME_MS = 120000;
static const fds_uint32_t MAX_POLL_EVENTS = 1024;
static const fds_uint32_t BUF_LEN = MAX_POLL_EVENTS * (sizeof(struct inotify_event) + NAME_MAX);

namespace fds {

void
DmTimeVolCatalog::notifyVolCatalogSync(BlobTxList::const_ptr sycndTxList) {
}

DmTimeVolCatalog::DmTimeVolCatalog(const std::string &name, fds_threadpool &tp)
        : Module(name.c_str()), opSynchronizer_(tp),
        config_helper_(g_fdsprocess->get_conf_helper()), tp_(tp), stopLogMonitoring_(false),
        logMonitorThread_(std::bind(&DmTimeVolCatalog::monitorLogs, this))
{
    volcat = DM_VOLUME_CATALOG_TYPE::ptr(new DM_VOLUME_CATALOG_TYPE("DM Volume Catalog"));

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
        commitLogs_[voldesc.volUUID] = boost::make_shared<DmCommitLog>("DM", voldesc.volUUID,
                voldesc.maxObjSizeInBytes);
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
            incrObjRefCount(voldesc.srcVolumeId, voldesc.volUUID, it.first, it.second);
            // tp_.schedule(&DmTimeVolCatalog::incrObjRefCount, this, voldesc.srcVolumeId,
            //         voldesc.volUUID, it.first, it.second);
        }
    }

    return rc;
}

void
DmTimeVolCatalog::incrObjRefCount(fds_volid_t srcVolId, fds_volid_t destVolId,
        fds_token_id token, boost::shared_ptr<std::vector<fpi::FDS_ObjectIdType> > objIds) {
    // TODO(umesh): this code is similar to DataMgr::expungeObject() code.
    // So it inherits all its limitations. Following things need to be considered in future:
    // 1. what if volume association is removed for OID while snapshot is being taken
    // 2. what if call to increment ref count fails
    // 3. whether to do it in background/ foreground thread

    OMgrClient * omClient = dataMgr->omClient;
    fds_verify(omClient);

    // Create message
    fpi::AddObjectRefMsgPtr addObjReq(new AddObjectRefMsg());
    addObjReq->srcVolId = srcVolId;
    addObjReq->destVolId = destVolId;
    addObjReq->objIds = *objIds;

    // for (auto it : *objIds) {
    //     addObjReq->objIds.push_back(it);
    // }

    const DLT * dlt = omClient->getCurrentDLT();
    fds_verify(dlt);

    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
            boost::make_shared<DltObjectIdEpProvider>(dlt->getNodes(token)));
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::AddObjectRefMsg), addObjReq);
    asyncReq->setTimeoutMs(10000);
    asyncReq->invoke();
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
    DmCommitLog::ptr commitLog;
    COMMITLOG_GET(volId, commitLog);
#ifdef ACTIVE_TX_IN_WRITE_BATCH
    return commitLog->updateTx(txDesc, objList);
#else
    auto boList = boost::make_shared<const BlobObjList>(objList);
    return commitLog->updateTx(txDesc, boList);
#endif
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
    CommitLogTx::ptr commit_data = commitLog->commitTx(txDesc, e);
    if (e.ok()) {
        e = doCommitBlob(volid, blob_version, commit_data);
    }
    cb(e, blob_version, commit_data->blobObjList, commit_data->metaDataList);
}

Error
DmTimeVolCatalog::doCommitBlob(fds_volid_t volid, blob_version_t & blob_version,
        CommitLogTx::ptr commit_data) {
    Error e;
    if (commit_data->blobDelete) {
        e = volcat->deleteBlob(volid, commit_data->name, commit_data->blobVersion);
        blob_version = commit_data->blobVersion;
    } else {
#ifdef ACTIVE_TX_IN_WRITE_BATCH
        e = volcat->putBlob(volid, commit_data->name, commit_data->blobSize,
                commit_data->metaDataList, commit_data->wb,
                0 != (commit_data->blobMode & blob::TRUNCATE));
#else
        if (commit_data->blobObjList && !commit_data->blobObjList->empty()) {
            if (commit_data->blobMode & blob::TRUNCATE) {
                commit_data->blobObjList->setEndOfBlob();
            }
            e = volcat->putBlob(volid, commit_data->name, commit_data->metaDataList,
                    commit_data->blobObjList, commit_data->txDesc);
        } else {
            e = volcat->putBlobMeta(volid, commit_data->name,
                    commit_data->metaDataList, commit_data->txDesc);
        }
#endif
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

void DmTimeVolCatalog::getDirChildren(const std::string & parent,
        std::vector<std::string> & children, bool dirs /* = true */) {
    fds_verify(!parent.empty());
    DIR * dirp = opendir(parent.c_str());
    if (!dirp) {
        LOGCRITICAL << "Failed to open directory '" << parent << "', error='"
                << errno << "'";
        return;
    }

    struct dirent entry;
    for (auto * dp = &entry; 0 == readdir_r(dirp, &entry, &dp) && dp; ) {
        if ((dirs && DT_DIR != entry.d_type) || (!dirs && DT_DIR == entry.d_type)) {
            continue;
        } else if (0 != strncmp(entry.d_name, ".", 1) && 0 != strncmp(entry.d_name, "..", 2)) {
            std::string tmpStr(entry.d_name);
            children.push_back(tmpStr);
        }
    }
    closedir(dirp);
}

void DmTimeVolCatalog::monitorLogs() {
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    const std::string dmDir = root->dir_user_repo_dm();
    FdsRootDir::fds_mkdir(dmDir.c_str());
    FdsRootDir::fds_mkdir(root->dir_timeline_dm().c_str());

    // initialize inotify
    int fd = inotify_init();
    if (fd < 0) {
        LOGCRITICAL << "Failed to initialize inotify, error='" << errno << "'";
        return;
    }

    std::set<std::string> watched;
    int wd = inotify_add_watch(fd, dmDir.c_str(), IN_CREATE);
    if (wd < 0) {
        LOGCRITICAL << "Failed to add watch for directory '" << dmDir << "'";
        return;
    }
    watched.insert(dmDir);

    // epoll related calls
    int efd = epoll_create(sizeof(fd));
    if (efd < 0) {
        LOGCRITICAL << "Failed to create epoll file descriptor, error='" << errno << "'";
        return;
    }
    struct epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLET;
    int ecfg = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
    if (ecfg < 0) {
        LOGCRITICAL << "Failed to configure epoll interface!";
        return;
    }

    char buffer[BUF_LEN] = {0};
    fds_bool_t processedEvent = false;
    fds_bool_t eventReady = false;

    while (!stopLogMonitoring_) {
        do {
            processedEvent = false;
            std::vector<std::string> volDirs;
            getDirChildren(dmDir, volDirs);

            for (const auto & d : volDirs) {
                std::string volPath = dmDir + d + "/";
                std::vector<std::string> catFiles;
                getDirChildren(volPath, catFiles, false);

                for (const auto & f : catFiles) {
                    if (0 == f.find(leveldb::DEFAULT_ARCHIVE_PREFIX)) {
                        LOGDEBUG << "Found leveldb archive file '" << volPath << f << "'";
                        std::string volTLPath = root->dir_timeline_dm() + d + "/";
                        FdsRootDir::fds_mkdir(volTLPath.c_str());

                        std::string srcFile = volPath + f;
                        std::string cpCmd = "cp -f " + srcFile + " " + volTLPath;
                        LOGDEBUG << "Running command: '" << cpCmd << "'";
                        fds_int32_t rc = std::system(cpCmd.c_str());
                        if (!rc) {
                            fds_verify(0 == unlink(srcFile.c_str()));
                            processedEvent = true;
                        } else {
                            LOGWARN << "Failed to copy file '" << srcFile << "' to directory '"
                                    << volTLPath;
                        }
                    }
                }

                std::set<std::string>::const_iterator iter = watched.find(volPath);
                if (watched.end() == iter) {
                    if ((wd = inotify_add_watch(fd, volPath.c_str(), IN_CREATE)) < 0) {
                        LOGCRITICAL << "Failed to add watch for directory '" << volPath << "'";
                        continue;
                    } else {
                        LOGDEBUG << "Watching directory '" << volPath << "'";
                        processedEvent = true;
                        watched.insert(volPath);
                    }
                }
            }

            if (stopLogMonitoring_) {
                return;
            }
        } while (processedEvent);

        do {
            eventReady = false;
            errno = 0;
            fds_int32_t fdCount = epoll_wait(efd, &ev, MAX_POLL_EVENTS, POLL_WAIT_TIME_MS);
            if (fdCount < 0) {
                if (EINTR != errno) {
                    LOGCRITICAL << "epoll_wait() failed, error='" << errno << "'";
                    LOGCRITICAL << "Stopping commit log monitoring...";
                    return;
                }
            } else if (0 == fdCount) {
                // timeout, refresh
                eventReady = true;
            } else {
                int len = read(fd, buffer, BUF_LEN);
                if (len < 0) {
                    LOGWARN << "Failed to read inotify event, error='" << errno << "'";
                    continue;
                }

                for (char * p = buffer; p < buffer + len; ) {
                    struct inotify_event * event = reinterpret_cast<struct inotify_event *>(p);
                    if (event->mask | IN_ISDIR || 0 == strncmp(event->name,
                            leveldb::DEFAULT_ARCHIVE_PREFIX.c_str(),
                            strlen(leveldb::DEFAULT_ARCHIVE_PREFIX.c_str()))) {
                        eventReady = true;
                        break;
                    }
                    p += sizeof(struct inotify_event) + event->len;
                }
            }
            if (stopLogMonitoring_) {
                return;
            }
        } while (!eventReady);
    }
}

Error DmTimeVolCatalog::dmReplayCatJournalOps(Catalog *destCat,
        const std::vector<std::string> &files, fds_uint64_t timelineTime) {
    Error rc(ERR_DM_REPLAY_JOURNAL);

    fds_uint64_t ts = 0;
    for (const auto &f : files) {
        for (leveldb::CatJournalIterator iter(f); iter.isValid(); iter.Next()) {
            leveldb::WriteBatch & wb = iter.GetBatch();
            ts = getWriteBatchTimestamp(wb);
            if (!ts) {
                LOGDEBUG << "Error getting the write batch time stamp";
            }
            if ((ts / 1000 * 1000) <= timelineTime) {
                rc = destCat->Update(&wb);
            }
        }
    }

    return rc;
}
Error DmTimeVolCatalog::dmGetCatJournalStartTime(const std::string &logfile,
        fds_uint64_t *journal_time) {
    Error err(ERR_DM_JOURNAL_TIME);

    *journal_time = 0;
    for (leveldb::CatJournalIterator iter(logfile); iter.isValid(); ) {
        *journal_time = getWriteBatchTimestamp(iter.GetBatch());
        break;
    }

    return *journal_time ? ERR_OK : err;
}
}  // namespace fds

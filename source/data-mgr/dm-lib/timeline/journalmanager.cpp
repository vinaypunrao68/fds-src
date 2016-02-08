/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

extern "C" {
#include <sys/types.h>
#include <sys/inotify.h>
#include <dirent.h>
}
#include <DataMgr.h>
#include <leveldb/cat_journal.h>
#include <util/path.h>
#include <dm-vol-cat/DmPersistVolDB.h>
namespace fds { namespace timeline {

static const fds_uint32_t POLL_WAIT_TIME_MS = 120000;
static const fds_uint32_t MAX_POLL_EVENTS = 1024;
static const fds_uint32_t BUF_LEN = MAX_POLL_EVENTS * (sizeof(struct inotify_event) + NAME_MAX);

JournalManager::JournalManager(fds::DataMgr* dm) : dm(dm),fStopLogMonitoring(false) {
    inotifyFd = -1;
    if (dm->features.isTimelineEnabled()) {
        logMonitor.reset(new std::thread(
            std::bind(&JournalManager::monitorLogs, this)));
    }
}

JournalManager::~JournalManager() {
    stopMonitoring();
}

void JournalManager::stopMonitoring() {
    if (dm->features.isTimelineEnabled()) {
        fStopLogMonitoring = true;
        if (inotifyFd > 0) {
            close(inotifyFd);
        }
        logMonitor->join();
    }
}

Error JournalManager::replayTransactions(Catalog& destCat,
                                         const std::vector<std::string> &files,
                                         util::TimeStamp fromTime,
                                         util::TimeStamp toTime) {
    Error err(ERR_DM_REPLAY_JOURNAL);

    fds_uint64_t ts = 0;
    for (const auto &f : files) {
        for (leveldb::CatJournalIterator iter(f); iter.isValid(); iter.Next()) {
            leveldb::WriteBatch & wb = iter.GetBatch();
            ts = getWriteBatchTimestamp(wb);
            if (!ts) {
                LOGDEBUG << "Error getting the write batch time stamp";
                break;
            }
            if (ts > toTime) {
                // we don't care about further records.
                break;
            }
            if (ts >= fromTime && ts <= toTime) {
                err = destCat.Update(&wb);
            }
        }
    }
    return err;
}

Error JournalManager::replayTransactions(fds_volid_t srcVolId,
                                         fds_volid_t destVolId,
                                         util::TimeStamp fromTime,
                                         util::TimeStamp toTime) {
    Error err(ERR_INVALID);
    std::vector<timeline::JournalFileInfo> vecJournalInfos;
    std::vector<std::string> journalFiles;
    dm->timelineMgr->getDB()->getJournalFiles(srcVolId, fromTime, toTime, vecJournalInfos);

    journalFiles.reserve(vecJournalInfos.size());
    for (auto& item : vecJournalInfos) {
        journalFiles.push_back(std::move(item.journalFile));
    }

    if (journalFiles.empty()) {
        LOGDEBUG << "no matching journal files to be replayed from"
                 << " srcvol:" << srcVolId << " onto vol:" << destVolId;
        return ERR_OK;
    } else {
        LOGNORMAL << "[" << journalFiles.size() << "] journal files will be replayed from"
                  << " srcvol:" << srcVolId << " onto vol:" << destVolId;
    }

    // get the correct catalog
    DmVolumeCatalog::ptr volDirPtr =  boost::dynamic_pointer_cast
            <DmVolumeCatalog>(dm->timeVolCat_->queryIface());

    if (volDirPtr.get() == NULL) {
        LOGERROR << "unable to get the vol dir ptr";
        return ERR_NOT_FOUND;
    }

    DmPersistVolCat::ptr persistVolDirPtr = volDirPtr->getVolume(destVolId);
    if (persistVolDirPtr.get() == NULL) {
        LOGERROR << "unable to get the persist vol dir ptr for vol:" << destVolId;
        return ERR_NOT_FOUND;
    }

    DmPersistVolDB::ptr voldDBPtr = boost::dynamic_pointer_cast
            <DmPersistVolDB>(persistVolDirPtr);
    Catalog* catalog = voldDBPtr->getCatalog();

    if (NULL == catalog) {
        LOGERROR << "unable to catalog for vol:" << destVolId;
        return ERR_NOT_FOUND;
    }

    return replayTransactions(*catalog, journalFiles, fromTime, toTime);
}

Error JournalManager::getJournalStartTime(const std::string &logfile,
                                          util::TimeStamp& startTime) {
    startTime = 0;
    for (leveldb::CatJournalIterator iter(logfile); iter.isValid(); ) {
        startTime = getWriteBatchTimestamp(iter.GetBatch());
        break;
    }

    return startTime ? ERR_OK : ERR_DM_JOURNAL_TIME;
}

/**
 * This thread will monitor filesystem for archived files. Whenever catalog journal
 * files are archived, the thread will wake up and copy the file to timeline directory.
 * The timeline directory will hold all journal files for all volumes.
 */
void JournalManager::monitorLogs() {
    LOGNORMAL << "journal log monitoring started";
    const FdsRootDir *root = dm->getModuleProvider()->proc_fdsroot();
    const std::string dmDir = root->dir_sys_repo_dm();
    FdsRootDir::fds_mkdir(dmDir.c_str());
    FdsRootDir::fds_mkdir(root->dir_timeline_dm().c_str());

    // initialize inotify
    int inotifyFd = inotify_init();
    if (inotifyFd < 0) {
        LOGCRITICAL << "Failed to initialize inotify, error= '" << errno << "'";
        return;
    }

    std::set<std::string> watched;
    int wd = inotify_add_watch(inotifyFd, dmDir.c_str(), IN_CREATE);
    if (wd < 0) {
        LOGCRITICAL << "Failed to add watch for directory '" << dmDir << "'";
        return;
    }
    watched.insert(dmDir);

    // epoll related calls
    int efd = epoll_create(sizeof(inotifyFd));
    if (efd < 0) {
        LOGCRITICAL << "Failed to create epoll file descriptor, error= '" << errno << "'";
        return;
    }
    struct epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLET;
    int ecfg = epoll_ctl(efd, EPOLL_CTL_ADD, inotifyFd, &ev);
    if (ecfg < 0) {
        LOGCRITICAL << "Failed to configure epoll interface!";
        return;
    }

    char buffer[BUF_LEN] = {0};
    fds_bool_t processedEvent = false;
    fds_bool_t eventReady = false;

    while (!fStopLogMonitoring) {
        do {
            processedEvent = false;
            std::vector<std::string> volDirs;
            util::getSubDirectories(dmDir, volDirs);
            LOGDEBUG << "monitoring : " << dmDir;

            for (const auto & d : volDirs) {
                std::string volPath = dmDir + d + "/";
                std::vector<std::string> catFiles;
                util::getFiles(volPath, catFiles);

                for (const auto & f : catFiles) {
                    if (0 == f.find(leveldb::DEFAULT_ARCHIVE_PREFIX)) {
                        LOGDEBUG << "Found leveldb archive file '" << volPath << f << "'";
                        std::string volTLPath = root->dir_timeline_dm() + d + "/";
                        FdsRootDir::fds_mkdir(volTLPath.c_str());

                        std::string srcFile = volPath + f;
                        std::string destFile = volTLPath + f + ".gz";
                        TimeStamp startTime = 0;
                        getJournalStartTime(srcFile, startTime);
                        std::string zipCmd = "gzip  --stdout " + srcFile + " > " + destFile;
                        LOGDEBUG << "running command: [" << zipCmd << "]";
                        auto rc = std::system(zipCmd.c_str());

                        if (!rc) {
                            fds_verify(0 == unlink(srcFile.c_str()));
                            processedEvent = true;
                            fds_volid_t volId (std::atoll(d.c_str()));
                            dm->timelineMgr->getDB()->addJournalFile(volId, startTime, destFile);
                        } else {
                            LOGWARN << "command failed [" << zipCmd << "], error:" << rc << ":" << strerror(rc);
                        }
                    }
                }

                std::set<std::string>::const_iterator iter = watched.find(volPath);
                if (watched.end() == iter) {
                    if ((wd = inotify_add_watch(inotifyFd, volPath.c_str(), IN_CREATE)) < 0) {
                        LOGCRITICAL << "Failed to add watch for directory '" << volPath << "'";
                        continue;
                    } else {
                        LOGDEBUG << "Watching directory '" << volPath << "'";
                        processedEvent = true;
                        watched.insert(volPath);
                    }
                }
            }

            if (fStopLogMonitoring) {
                return;
            }
        } while (processedEvent);

        // now check for each volume if the commit log time has been exceeded
        removeExpiredJournals();

        do {
            eventReady = false;
            errno = 0;
            fds_int32_t fdCount = epoll_wait(efd, &ev, MAX_POLL_EVENTS, POLL_WAIT_TIME_MS);
            if (fStopLogMonitoring) {
                return;
            }
            if (fdCount < 0) {
                if (EINTR != errno) {
                    LOGCRITICAL << "epoll_wait() failed, error= '" << errno << "'";
                    LOGCRITICAL << "Stopping commit log monitoring...";
                    return;
                }
            } else if (0 == fdCount) {
                // timeout, refresh
                eventReady = true;
            } else {
                int len = read(inotifyFd, buffer, BUF_LEN);
                if (len < 0) {
                    LOGWARN << "Failed to read inotify event, error= '" << errno << "'";
                    continue;
                }

                for (char * p = buffer; p < buffer + len; ) {
                    struct inotify_event * event = reinterpret_cast<struct inotify_event *>(p);
                    if (event->mask | IN_ISDIR
                        || 0 == strncmp(event->name,
                                        leveldb::DEFAULT_ARCHIVE_PREFIX.c_str(),
                                        strlen(leveldb::DEFAULT_ARCHIVE_PREFIX.c_str()))) {
                        eventReady = true;
                        break;
                    }
                    p += sizeof(struct inotify_event) + event->len;
                }
            }
        } while (!eventReady);
    }
}

void JournalManager::removeExpiredJournals() {
    TimeStamp now = fds::util::getTimeStampMicros();
    std::vector<timeline::JournalFileInfo> vecJournalFiles;
    TimeStamp retention = 0;
    int rc;

    // get the list of volumes in the system
    std::vector<fds_volid_t> vecVolIds;
    dm->getActiveVolumes(vecVolIds);

    for (const auto& volid : vecVolIds) {
        vecJournalFiles.clear();
        const VolumeDesc *volumeDesc = dm->getVolumeDesc(volid);
        if (!volumeDesc) {
            LOGWARN << "unable to get voldesc for vol:" << volid;
            continue;
        }
        retention = volumeDesc->contCommitlogRetention * 1000 * 1000;
        if (retention > 0) {
            dm->timelineMgr->getDB()->removeOldJournalFiles(volid,
                                                            now - retention,
                                                            vecJournalFiles);
            if (!vecJournalFiles.empty()) {
                LOGNORMAL << "[" << vecJournalFiles.size() << "] files will be removed for vol:" << volid;
            } else {
                LOGDEBUG << "No journals to be removed for vol:" << volid;
            }

            for (const auto& journal : vecJournalFiles) {
                rc = unlink(journal.journalFile.c_str());
                if (rc) {
                    LOGERROR << "unable to remove old archive : " << journal.journalFile;
                } else {
                    LOGNORMAL << "journal file removed successfully : " << journal.journalFile;
                }
            }
        }
    }
}

}  // namespace timeline
}  // namespace fds

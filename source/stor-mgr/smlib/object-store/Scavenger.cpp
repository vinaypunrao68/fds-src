/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <sys/statvfs.h>
#include <set>
#include <vector>
#include <string>
#include <util/Log.h>
#include <fds_assert.h>
#include <object-store/TokenCompactor.h>
#include <object-store/ObjectPersistData.h>
#include <object-store/Scavenger.h>

using diskio::DiskStat;

namespace fds {

#define SCAV_TIMER_SECONDS (2*3600)  // 2 hours
#define DEFAULT_MAX_DISKS_COMPACTING (2)


ScavControl::ScavControl(const std::string &modName,
                         SmIoReqHandler *data_store,
                         SmPersistStoreHandler *persist_store)
        : Module(modName.c_str()),
          scav_lock("Scav Lock"),
          max_disks_compacting(DEFAULT_MAX_DISKS_COMPACTING),
          dataStoreReqHandler(data_store),
          persistStoreGcHandler(persist_store),
          scav_timer(new FdsTimer()),
          scav_timer_task(new ScavTimerTask(*scav_timer, this))
{
    enabled = ATOMIC_VAR_INIT(false);
}

ScavControl::~ScavControl() {
    scav_timer->destroy();
    for (DiskScavTblType::iterator it = diskScavTbl.begin();
         it != diskScavTbl.end();
         ++it) {
        DiskScavenger *diskScav = it->second;
        if (diskScav != NULL) {
            delete diskScav;
        }
    }
}

int
ScavControl::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void
ScavControl::mod_startup() {
}

void
ScavControl::mod_shutdown() {
}

void ScavControl::updateDiskStats()
{
    LOGNORMAL << "Updating disk stats";
    fds_mutex::scoped_lock l(scav_lock);
    for (DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
         cit != diskScavTbl.cend();
         ++cit) {
        DiskScavenger *diskScav = cit->second;
        if (diskScav != NULL) {
            diskScav->updateDiskStats();
        }
    }
}

void
ScavControl::createDiskScavengers(const SmDiskMap::const_ptr& diskMap) {
    // TODO(Anna) this method still assumes that once we discover disks
    // they never change (no disks added/removed). So we are going to populate
    // diskScavTable once and keep it unchanged for now. Will have to revisit
    // this and make more dynamic
    fds_mutex::scoped_lock l(scav_lock);
    if (diskScavTbl.size() == 0) {
        DiskIdSet hddIds = diskMap->getDiskIds(diskio::diskTier);
        DiskIdSet ssdIds = diskMap->getDiskIds(diskio::flashTier);
        // create scavengers for HDDs
        for (DiskIdSet::const_iterator cit = hddIds.cbegin();
             cit != hddIds.cend();
             ++cit) {
            DiskScavenger *diskScav = new DiskScavenger(*cit, diskio::diskTier,
                                                        dataStoreReqHandler,
                                                        persistStoreGcHandler,
                                                        diskMap);
            fds_verify(diskScavTbl.count(*cit) == 0);
            diskScavTbl[*cit] = diskScav;
            LOGNORMAL << "Added scavenger for HDD " << *cit;
        }
        // create scavengers got SSDs
        // TODO(Anna) commenting out SSD scavenger, until we put back tiering
        /*
        for (DiskIdSet::const_iterator cit = ssdIds.cbegin();
             cit != ssdIds.cend();
             ++cit) {
            DiskScavenger *diskScav = new DiskScavenger(*cit, diskio::flashTier,
                                                        dataStoreReqHandler,
                                                        persistStoreGcHandler,
                                                        diskMap);
            fds_verify(diskScavTbl.count(*cit) == 0);
            diskScavTbl[*cit] = diskScav;
            LOGNORMAL << "Added scavenger for SSD " << *cit;
        }
        */
    }
}

Error ScavControl::enableScavenger(const SmDiskMap::const_ptr& diskMap)
{
    Error err(ERR_OK);
    fds_bool_t expect = false;

    // if this is called for the first time and diskScavTbl is empty,
    // populate diskScavTable first (before changing enabled to true,
    // so that the scavengers do not start to early)
    createDiskScavengers(diskMap);

    if (!std::atomic_compare_exchange_strong(&enabled, &expect, true)) {
        LOGNOTIFY << "Scavenger cycle is already enabled, nothing else to do";
        return ERR_SM_GC_ENABLED;
    }
    LOGNOTIFY << "Enabling Scavenger";

    // start timer to update disk stats
    if (!scav_timer->scheduleRepeated(scav_timer_task,
                                      std::chrono::seconds(SCAV_TIMER_SECONDS))) {
        LOGWARN << "Failed to schedule timer for updating disks stats! "
                << " will not run automatic GC, only manual";
        return ERR_SM_AUTO_GC_FAILED;
    }

    return err;
}

void ScavControl::disableScavenger()
{
    fds_bool_t expect = true;

    // stop the periodic timer that does automatic scavenging
    // we are doing it before reseting 'enabled' because calling it
    // 2 times is ok, and if there if enable is called at the same time
    // (before we reset enabled) it will be a noop
    scav_timer->cancel(scav_timer_task);

    // tell disk scavengers to stop scavenging if it is currently in progress
    stopScavengeProcess();

    if (std::atomic_compare_exchange_strong(&enabled, &expect, false)) {
        LOGNOTIFY << "Disabled Scavenger; will not be able to start scavenger"
                  << " process manually as well until scavenger is enabled";
    } else {
        LOGNOTIFY << "Scavenger was already disabled";
    }
}

void ScavControl::startScavengeProcess()
{
    fds_bool_t _enabled = std::atomic_load(&enabled);
    if (!_enabled) {
        LOGNOTIFY << "Cannot start Scavenger process because scavenger"
                  << " is disabled";
        return;
    }

    LOGNORMAL << "Starting Scavenger cycle";
    fds_mutex::scoped_lock l(scav_lock);
    for (DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
         cit != diskScavTbl.cend();
         ++cit) {
        DiskScavenger *diskScav = cit->second;
        if (diskScav != NULL) {
            diskScav->startScavenge();
        }
    }
}

void ScavControl::stopScavengeProcess()
{
    fds_bool_t _enabled = std::atomic_load(&enabled);
    if (!_enabled) {
        LOGNORMAL << "Scavenger is disabled, so nothing to stop";
        return;
    }

    LOGNORMAL << "Stop Scavenger cycle... ";
    fds_mutex::scoped_lock l(scav_lock);
    // TODO(xxx) stop everything for now
    for (DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
         cit != diskScavTbl.cend();
         ++cit) {
        DiskScavenger *diskScav = cit->second;
        if (diskScav != NULL) {
            diskScav->stopScavenge();
        }
    }
}

DiskScavenger::DiskScavenger(fds_uint16_t _disk_id,
                             diskio::DataTier _tier,
                             SmIoReqHandler *data_store,
                             SmPersistStoreHandler* persist_store,
                             const SmDiskMap::const_ptr& diskMap)
        : disk_id(_disk_id),
          disk_scav_lock("Disk-Scav Lock"),
          smDiskMap(diskMap),
          persistStoreGcHandler(persist_store),
          tier(_tier),
          scav_policy()  // default policy
{
    next_token = 0;

    in_progress = ATOMIC_VAR_INIT(false);
    for (fds_uint32_t i = 0; i < scav_policy.proc_max_tokens; ++i) {
        tok_compactor_vec.push_back(TokenCompactorPtr(new TokenCompactor(data_store,
                                                                         persist_store)));
    }
}

DiskScavenger::~DiskScavenger() {
}

void DiskScavenger::setPolicy(const DiskScavPolicyInternal& policy) {
    scav_policy = policy;
}

void DiskScavenger::setProcMaxTokens(fds_uint32_t max_toks) {
    scav_policy.proc_max_tokens = max_toks;
}

Error
DiskScavenger::getDiskStats(diskio::DiskStat* retStat) {
    Error err(ERR_OK);
    struct statvfs statbuf;
    std::string diskPath = smDiskMap->getDiskPath(disk_id);
    if (statvfs(diskPath.c_str(), &statbuf) < 0) {
        return fds::Error(fds::ERR_DISK_READ_FAILED);
    }

    // aggregate token stats for total deleted bytes
    SmTokenSet diskToks = smDiskMap->getSmTokens(disk_id);
    fds_uint64_t totDeletedBytes = 0;
    for (SmTokenSet::const_iterator cit = diskToks.cbegin();
         cit != diskToks.cend();
         ++cit) {
        diskio::TokenStat stat;
        persistStoreGcHandler->getSmTokenStats(*cit, tier, &stat);
        totDeletedBytes += stat.tkn_reclaim_size;
        LOGDEBUG << "Disk id " << disk_id << " SM token " << *cit
                 << " reclaim bytes " << stat.tkn_reclaim_size;
    }

    fds_verify(retStat);
    (*retStat).dsk_tot_size = statbuf.f_blocks * statbuf.f_bsize;
    (*retStat).dsk_avail_size = statbuf.f_bfree * statbuf.f_bsize;
    (*retStat).dsk_reclaim_size = totDeletedBytes;
    return err;
}

void DiskScavenger::updateDiskStats() {
    Error err(ERR_OK);
    DiskStat disk_stat;
    double tot_size;
    fds_uint32_t avail_percent;  // percent of available capacity
    fds_uint32_t token_reclaim_threshold;

    err = getDiskStats(&disk_stat);
    fds_verify(err.ok());
    tot_size = disk_stat.dsk_tot_size;
    avail_percent = (disk_stat.dsk_avail_size / tot_size) * 100;
    LOGDEBUG << "Tier " << tier << " disk " << disk_id
             << " total " << disk_stat.dsk_tot_size
             << ", avail " << disk_stat.dsk_avail_size << " ("
             << avail_percent << "%), reclaim " << disk_stat.dsk_reclaim_size;

    // Decide if we want to GC this disk and which tokens
    if (avail_percent < scav_policy.dsk_avail_threshold_1) {
        // we are going to GC this disk
        if (avail_percent < scav_policy.dsk_avail_threshold_2) {
            // we will GC all tokens
            token_reclaim_threshold = 0;
        } else {
            // we will GC only tokens that are worth to GC
            token_reclaim_threshold = scav_policy.tok_reclaim_threshold;
        }

        // start token compaction process
        startScavenge(token_reclaim_threshold);
    }
}

fds_bool_t DiskScavenger::getNextCompactToken(fds_token_id* tok_id) {
    fds_verify(tok_id);
    fds_bool_t found = false;
    *tok_id = 0;

    fds_mutex::scoped_lock l(disk_scav_lock);
    if (tokenDb.count(next_token) == 0) {
        // if we don't have this tok id, find the closes id > tok_id
        // since elements in the set are sorted, find first elm with id > tok_id
        for (std::set<fds_token_id>::const_iterator cit = tokenDb.cbegin();
             cit != tokenDb.cend();
             ++cit) {
            if (*cit > next_token) {
                *tok_id = *cit;
                next_token = (*tok_id)+1;
                found = true;
                break;
            }
        }
    } else {
        *tok_id = next_token;
        ++next_token;
        found = true;
    }
    LOGTRACE << "Disk " << disk_id << " has " << tokenDb.size()
             << " tokens " << " found next tok? " << found
             << " tok_id " << *tok_id;

    return found;
}

fds_bool_t DiskScavenger::isTokenCompacted(const fds_token_id& tok_id) {
    if (tokenDb.find(tok_id) != tokenDb.cend()) {
       return true;
    } else {
       return false;
    }
}

// Re-builds tokenDb with tokens that need to be scavenged
// TODO(xxx) pass in policy which tokens need to be scavenged
// for now all tokens we get from persistent layer
void DiskScavenger::findTokensToCompact(fds_uint32_t token_reclaim_threshold) {
    fds_uint32_t reclaim_percent;
    // note that we are not using lock here, because updateTokenDb()
    // and getNextCompactToken are serialized

    // reset tokenDb
    tokenDb.clear();

    // get all tokens that SM owns and that reside on this disk
    SmTokenSet diskToks = smDiskMap->getSmTokens(disk_id);

    // add tokens to tokenDb that we need to compact
    for (SmTokenSet::const_iterator cit = diskToks.cbegin();
         cit != diskToks.cend();
         ++cit) {
        diskio::TokenStat stat;
        persistStoreGcHandler->getSmTokenStats(*cit, tier, &stat);
        double tot_size = stat.tkn_tot_size;
        reclaim_percent = (stat.tkn_reclaim_size / tot_size) * 100;
        LOGDEBUG << "Disk " << disk_id << " token " << stat.tkn_id
                 << " total bytes " << stat.tkn_tot_size
                 << ", deleted bytes " << stat.tkn_reclaim_size
                 << " (" << reclaim_percent << "%)";

        if (stat.tkn_reclaim_size > 0) {
            if (reclaim_percent >= token_reclaim_threshold) {
                tokenDb.insert(stat.tkn_id);
            }
        }
    }
}

void DiskScavenger::startScavenge(fds_uint32_t token_reclaim_threshold) {
    fds_uint32_t i = 0;
    fds_token_id tok_id;
    fds_bool_t expect = false;
    if (!std::atomic_compare_exchange_strong(&in_progress, &expect, true)) {
        LOGNOTIFY << "Scavenger cycle is already running, ignoring start scavenge req";
        return;
    }

    // get list of tokens for this tier/disk from persistent layer
    findTokensToCompact(token_reclaim_threshold);
    if (tokenDb.size() == 0) {
        LOGNORMAL << "No tokens to compact on disk " << disk_id << " tier " << tier;
        std::atomic_exchange(&in_progress, false);
        return;
     }

    LOGNORMAL << "Scavenger process started for disk_id " << disk_id << " tier "
              << tier << " number of tokens " << tokenDb.size();
    next_token = 0;

    for (i = 0; i < scav_policy.proc_max_tokens; ++i) {
        fds_bool_t found = getNextCompactToken(&tok_id);
        if (!found) {
            LOGNORMAL << "Scavenger process finished for disk_id " << disk_id;
            std::atomic_exchange(&in_progress, false);
            break;
        }
        // TODO(Anna) commenting our isTokenInSyncMode -- need to revisit
        // when porting back token migration
        // if (!objStorMgr->isTokenInSyncMode(tok_id)) {
           tok_compactor_vec[i]->startCompaction(tok_id, disk_id, tier, std::bind(
               &DiskScavenger::compactionDoneCb, this,
               std::placeholders::_1, std::placeholders::_2));
           // }
    }
}

void DiskScavenger::stopScavenge() {
    fds_bool_t expect = true;
    if (std::atomic_compare_exchange_strong(&in_progress, &expect, false)) {
        LOGNOTIFY << "Stopping scavenger cycle...";
    } else {
        LOGNOTIFY << "Scavenger was not running for disk_id " << disk_id;
    }
}

void DiskScavenger::compactionDoneCb(fds_token_id token_id, const Error& error) {
    fds_token_id tok_id;
    fds_bool_t in_prog = std::atomic_load(&in_progress);

    LOGNORMAL << "Compaction done notif for token " << token_id
              << " disk_id " << disk_id << " result " << error;
    if (!in_prog) {
        // either stop scavenger was called, or we found no more tokens to compact
        return;
    }

    // find available token compactor and start compaction of next token
    if (error.ok()) {
        for (fds_uint32_t i = 0; i < scav_policy.proc_max_tokens; ++i) {
            if (tok_compactor_vec[i]->isIdle()) {
                fds_bool_t found = getNextCompactToken(&tok_id);
                if (!found) {
                    in_prog = false;
                    break;
                }
                // TODO(Anna) commenting our isTokenInSyncMode -- need to revisit
                // when porting back token migration
                // if (!objStorMgr->isTokenInSyncMode(tok_id)) {
                   tok_compactor_vec[i]->startCompaction(tok_id, disk_id, tier, std::bind(
                       &DiskScavenger::compactionDoneCb, this,
                       std::placeholders::_1, std::placeholders::_2));
                   // }
             }
        }
    } else {
        // lets not continue if error
        in_prog = false;
    }

    if (!in_prog) {
        fds_bool_t expect = true;
        if (std::atomic_compare_exchange_strong(&in_progress, &expect, false)) {
            LOGNORMAL << "Scavenger process finished for disk_id " << disk_id;
        }
    }
}

void ScavTimerTask::runTimerTask()
{
    fds_verify(scavenger);
    scavenger->updateDiskStats();
}

DiskScavPolicyInternal::DiskScavPolicyInternal(const DiskScavPolicyInternal& policy)
        : dsk_avail_threshold_1(policy.dsk_avail_threshold_1),
          dsk_avail_threshold_2(policy.dsk_avail_threshold_2),
          tok_reclaim_threshold(policy.tok_reclaim_threshold),
          proc_max_tokens(policy.proc_max_tokens) {
}

DiskScavPolicyInternal::DiskScavPolicyInternal()
        : dsk_avail_threshold_1(SCAV_DEFAULT_DSK_AVAIL_THRESHOLD_1),
          dsk_avail_threshold_2(SCAV_DEFAULT_DSK_AVAIL_THRESHOLD_2),
          tok_reclaim_threshold(SCAV_DEFAULT_TOK_RECLAIM_THRESHOLD),
          proc_max_tokens(SCAV_DEFAULT_PROC_MAX_TOKENS) {
}

DiskScavPolicyInternal&
DiskScavPolicyInternal::operator=(const DiskScavPolicyInternal& other) {
    dsk_avail_threshold_1 = other.dsk_avail_threshold_1;
    dsk_avail_threshold_2 = other.dsk_avail_threshold_2;
    tok_reclaim_threshold = other.tok_reclaim_threshold;
    proc_max_tokens = other.proc_max_tokens;
    return *this;
}

}  // namespace fds

/*                                                                                                                                          
 * Copyright 2014 Formation Data Systems, Inc.                                                                                              
 */

#include <set>
#include <vector>
#include <util/Log.h>
#include <fds_assert.h>
#include <StorMgr.h>
#include <TokenCompactor.h>
#include <persistent_layer/persistentdata.h>
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_io.h>
#include <Scavenger.h>

using diskio::DiskStat;

namespace fds {

#define SCAV_TIMER_SECONDS (2*3600)  // 2 hours

extern ObjectStorMgr *objStorMgr;
extern diskio::DataDiscoveryModule  dataDiscoveryMod;
extern diskio::DataIOModule gl_dataIOMod;

ScavControl::ScavControl(fds_uint32_t const num_thrds)
        : scav_lock("Scav Lock"),
          max_disks_compacting(num_thrds),
          scav_timer(new FdsTimer()),
          scav_timer_task(new ScavTimerTask(*scav_timer, this))
{
    enabled = ATOMIC_VAR_INIT(true);
    num_hdd = 0;
    num_ssd = 0;
    std::set<fds_uint16_t> hdd_ids;
    std::set<fds_uint16_t> ssd_ids;
    diskio::dataDiscoveryMod.get_hdd_ids(&hdd_ids);
    diskio::dataDiscoveryMod.get_ssd_ids(&ssd_ids);
    // create scavengers for HDDs
    for (std::set<fds_uint16_t>::const_iterator cit = hdd_ids.cbegin();
         cit != hdd_ids.cend();
         ++cit) {
        DiskScavenger *diskScav = new DiskScavenger(*cit, diskio::diskTier);
        diskScavTbl[*cit] = diskScav;
        ++num_hdd;
        LOGNORMAL << "Added scavenger for HDD " << *cit << " (" << num_hdd << ")";
    }
    // create scavengers got SSDs
    for (std::set<fds_uint16_t>::const_iterator cit = ssd_ids.cbegin();
         cit != ssd_ids.cend();
         ++cit) {
        DiskScavenger *diskScav = new DiskScavenger(*cit, diskio::flashTier);
        diskScavTbl[*cit] = diskScav;
        ++num_ssd;
        LOGNORMAL << "Added scavenger for SSD " << *cit << " (" << num_ssd << ")";
    }

    // start timer to update disk stats
    if (!scav_timer->scheduleRepeated(scav_timer_task,
                                      std::chrono::seconds(SCAV_TIMER_SECONDS))) {
        LOGWARN << "Failed to schedule timer for updating disks stats! "
                << " will not run automatic GC, only manual";
    }
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

void ScavControl::enableScavenger()
{
    fds_bool_t expect = false;
    if (!std::atomic_compare_exchange_strong(&enabled, &expect, true)) {
        LOGNOTIFY << "Scavenger cycle is already enabled, nothing else to do";
        return;
    }
    LOGNOTIFY << "Enabling Scavenger";

    // start the periodic timer that checks disk stats, etc. and starts
    // scavenging if needed
    if (!scav_timer->scheduleRepeated(scav_timer_task,
                                      std::chrono::seconds(SCAV_TIMER_SECONDS))) {
        LOGWARN << "Failed to schedule timer for updating disks stats! "
                << " will not run automatic GC, only manual";
    }
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
                             diskio::DataTier _tier)
        : disk_id(_disk_id),
          disk_scav_lock("Disk-Scav Lock"),
          tier(_tier),
          scav_policy()  // default policy
{
    next_token = 0;

    in_progress = ATOMIC_VAR_INIT(false);
    for (fds_uint32_t i = 0; i < scav_policy.proc_max_tokens; ++i) {
        tok_compactor_vec.push_back(TokenCompactorPtr(new TokenCompactor(objStorMgr)));
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

void DiskScavenger::updateDiskStats() {
    DiskStat disk_stat;
    double tot_size;
    fds_uint32_t avail_percent;  // percent of available capacity
    fds_uint32_t token_reclaim_threshold;

    diskio::gl_dataIOMod.get_disk_stats(tier, disk_id, &disk_stat);
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
    std::vector<diskio::TokenStat> token_stats;
    fds_uint32_t reclaim_percent;
    double tot_size;
    // note that we are not using lock here, because updateTokenDb()
    // and getNextCompactToken are serialized

    // reset tokenDb
    tokenDb.clear();

    // get all tokens with their stats from persistent layer
    diskio::gl_dataIOMod.get_disk_token_stats(tier, disk_id, &token_stats);

    // add tokens to tokenDb that we need to compact
    for (std::vector<diskio::TokenStat>::const_iterator cit = token_stats.cbegin();
         cit != token_stats.cend();
         ++cit) {
        tot_size = (*cit).tkn_tot_size;
        reclaim_percent = ((*cit).tkn_reclaim_size / tot_size) * 100;
        LOGDEBUG << "Disk " << disk_id << " token " << (*cit).tkn_id
                 << " total bytes " << (*cit).tkn_tot_size
                 << ", deleted bytes " << (*cit).tkn_reclaim_size
                 << " (" << reclaim_percent << "%)";

        if ((*cit).tkn_reclaim_size > 0) {
            if (reclaim_percent >= token_reclaim_threshold) {
                tokenDb.insert((*cit).tkn_id);
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
        if (!objStorMgr->isTokenInSyncMode(tok_id)) {
           tok_compactor_vec[i]->startCompaction(tok_id, disk_id, tier, std::bind(
               &DiskScavenger::compactionDoneCb, this,
               std::placeholders::_1, std::placeholders::_2));
        }
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
                if (!objStorMgr->isTokenInSyncMode(tok_id)) {
                   tok_compactor_vec[i]->startCompaction(tok_id, disk_id, tier, std::bind(
                       &DiskScavenger::compactionDoneCb, this,
                       std::placeholders::_1, std::placeholders::_2));
                 }
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

/*                                                                                                                                          
 * Copyright 2014 Formation Data Systems, Inc.                                                                                              
 */

#include <set>
#include <vector>
#include <util/Log.h>
#include <fds_assert.h>
#include <StorMgr.h>
#include <TokenCompactor.h>
#include <persistentdata.h>
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_io.h>
#include <Scavenger.h>

namespace fds {
#define SCAV_BITS_PER_TOKEN 8
extern ObjectStorMgr *objStorMgr;
extern DataDiscoveryModule  dataDiscoveryMod;

ScavControl::ScavControl(fds_uint32_t num_thrds)
        : scav_lock("Scav Lock"),
          max_disks_compacting(num_thrds)
{
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
        DiskScavenger *diskScav = new DiskScavenger(*cit, diskio::diskTier, 1);
        diskScavTbl[*cit] = diskScav;
        ++num_hdd;
        LOGNORMAL << "Added scavenger for HDD " << *cit << " (" << num_hdd << ")";
    }
    // create scavengers got SSDs
    for (std::set<fds_uint16_t>::const_iterator cit = ssd_ids.cbegin();
         cit != ssd_ids.cend();
         ++cit) {
        DiskScavenger *diskScav = new DiskScavenger(*cit, diskio::flashTier, 1);
        diskScavTbl[*cit] = diskScav;
        ++num_ssd;
        LOGNORMAL << "Added scavenger for SSD " << *cit << " (" << num_ssd << ")";
    }
}

ScavControl::~ScavControl() {
    for (DiskScavTblType::iterator it = diskScavTbl.begin();
         it != diskScavTbl.end();
         ++it) {
        DiskScavenger *diskScav = it->second;
        if (diskScav != NULL) {
            delete diskScav;
        }
    }
}

void ScavControl::startScavengeProcess(fds_bool_t b_all, diskio::DataTier tgt_tier)
{
    LOGNORMAL << "Starting Scavenger cycle for all? " << b_all
              << " or tier " << tgt_tier;

    fds_mutex::scoped_lock l(scav_lock);
    for (DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
         cit != diskScavTbl.cend();
         ++cit) {
        DiskScavenger *diskScav = cit->second;
        if ((diskScav != NULL) &&
            (b_all || diskScav->isTier(tgt_tier))) {
            diskScav->startScavenge();
        }
    }
}

void ScavControl::stopScavengeProcess()
{
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

fds::Error ScavControl::addTokenCompactor(fds_token_id tok_id, fds_uint16_t disk_id) {
    Error err(ERR_OK);
    DiskScavenger* diskScav = NULL;

    {  // lock
        fds_mutex::scoped_lock l(scav_lock);
        if (diskScavTbl.count(disk_id) > 0) {
            diskScav = diskScavTbl[disk_id];
        }
    }  // unlock

    if (diskScav != NULL) {
        fds_mutex::scoped_lock l(diskScav->disk_scav_lock);
        (diskScav->tokenDb).insert(tok_id);
    }
    return err;
}

fds::Error ScavControl::deleteTokenCompactor(fds_token_id tok_id, fds_uint16_t disk_id) {
    Error err(ERR_OK);
    DiskScavenger* diskScav = NULL;

    {  // lock
        fds_mutex::scoped_lock l(scav_lock);
        if (diskScavTbl.count(disk_id) > 0) {
            diskScav = diskScavTbl[disk_id];
        }
    }  // unlock

    if (diskScav) {
        fds_mutex::scoped_lock l(diskScav->disk_scav_lock);
        (diskScav->tokenDb).erase(tok_id);
    }

    return err;
}

DiskScavenger::DiskScavenger(fds_uint16_t _disk_id,
                             diskio::DataTier _tier,
                             fds_uint32_t proc_max_tokens)
        : disk_id(_disk_id),
          disk_scav_lock("Disk-Scav Lock"),
          tier(_tier)
{
    max_tokens_in_proc = proc_max_tokens;
    next_token = 0;

    in_progress = ATOMIC_VAR_INIT(false);
    for (fds_uint32_t i = 0; i < proc_max_tokens; ++i) {
        tok_compactor_vec.push_back(TokenCompactorPtr(new TokenCompactor(objStorMgr)));
    }
}

DiskScavenger::~DiskScavenger() {
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
    LOGDEBUG << "Disk " << disk_id << " has " << tokenDb.size()
             << " tokens " << " found next tok? " << found
             << " next_token " << next_token
             << " tok_id " << *tok_id;

    return found;
}

void DiskScavenger::startScavenge() {
    fds_uint32_t i = 0;
    fds_token_id tok_id;
    fds_bool_t expect = false;
    if (!std::atomic_compare_exchange_strong(&in_progress, &expect, true)) {
        LOGNOTIFY << "Scavenger cycle is already running, ignoring start scavenge req";
        return;
    }
    LOGNORMAL << "Starting scavenger for disk " << disk_id << " tier " << tier;
    next_token = 0;

    for (i = 0; i < max_tokens_in_proc; ++i) {
        fds_bool_t found = getNextCompactToken(&tok_id);
        if (!found) {
            LOGNORMAL << "Scavenger process finished for disk_id " << disk_id;
            std::atomic_exchange(&in_progress, false);
            break;
        }
        tok_compactor_vec[i]->startCompaction(
            tok_id, disk_id, tier, SCAV_BITS_PER_TOKEN, std::bind(
                &DiskScavenger::compactionDoneCb, this,
                std::placeholders::_1, std::placeholders::_2));
    }
}

void DiskScavenger::stopScavenge() {
    fds_bool_t expect = true;
    if (std::atomic_compare_exchange_strong(&in_progress, &expect, false)) {
        LOGNOTIFY << "Stopping scavenger cycle...";
    } else {
        LOGNOTIFY << "Scavenger was not running, so nothing to stop";
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
        for (fds_uint32_t i = 0; i < max_tokens_in_proc; ++i) {
            if (tok_compactor_vec[i]->isIdle()) {
                fds_bool_t found = getNextCompactToken(&tok_id);
                if (!found) {
                    in_prog = false;
                    break;
                }
                tok_compactor_vec[i]->startCompaction(
                    tok_id, disk_id, tier, SCAV_BITS_PER_TOKEN, std::bind(
                        &DiskScavenger::compactionDoneCb, this,
                        std::placeholders::_1, std::placeholders::_2));
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


}  // namespace fds

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <sys/statvfs.h>
#include <set>
#include <vector>
#include <string>
#include <fds_process.h>
#include <util/Log.h>
#include <fds_assert.h>
#include <StorMgr.h>
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
          nextDiskToCompact(SM_INVALID_DISK_ID),
          intervalSeconds(SCAV_TIMER_SECONDS),
          dataStoreReqHandler(data_store),
          persistStoreGcHandler(persist_store),
          noPersistScavStats(false),
          verifyData(false),
          scav_timer(new FdsTimer()),
          scav_timer_task(new ScavTimerTask(*scav_timer, this))
{
    enabled = ATOMIC_VAR_INIT(false);
    whoDisabledMe = SM_CMD_INITIATOR_NOT_SET;
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
    max_disks_compacting = g_fdsprocess->get_fds_config()->get<uint32_t>("fds.sm.scavenger.max_disks_compact",
                                                                         DEFAULT_MAX_DISKS_COMPACTING);
    intervalSeconds = g_fdsprocess->get_fds_config()->get<uint32_t>("fds.sm.scavenger.interval_seconds",
                                                                    SCAV_TIMER_SECONDS);

    LOGDEBUG << "Scavenger will be compacting at most " << max_disks_compacting
             << " disks at a time; scavenger interval " << intervalSeconds << " seconds";
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
    fds_uint32_t count = 0;
    LOGNORMAL << "Updating disk stats";
    fds_mutex::scoped_lock l(scav_lock);
    // start first max_disks_compacting disk scavengers
    if (nextDiskToCompact != SM_INVALID_DISK_ID) {
        LOGWARN << "Looks like GC is already in progress.. not going to start GC ";
        return;
    }
    for (DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
         cit != diskScavTbl.cend();
         ++cit) {
        DiskScavenger *diskScav = cit->second;
        if (count >= max_disks_compacting) {
            nextDiskToCompact = diskScav->diskId();
            ++count;  // so that we can check if we updated next disk ID
            break;
        }
        if (diskScav != nullptr) {
            fds_bool_t started = diskScav->updateDiskStats(verifyData, std::bind(
                &ScavControl::diskCompactionDoneCb, this,
                std::placeholders::_1, std::placeholders::_2));
            if (started) {
                ++count;
            }
        }
    }
    if (count <= max_disks_compacting) {
        nextDiskToCompact = SM_INVALID_DISK_ID;
    }
}

Error
ScavControl::createDiskScavengers(const SmDiskMap::const_ptr& diskMap) {
    // TODO(Anna) this method still assumes that once we discover disks
    // they never change (no disks added/removed). So we are going to populate
    // diskScavTable once and keep it unchanged for now. Will have to revisit
    // this and make more dynamic
    fds_mutex::scoped_lock l(scav_lock);
    if (diskScavTbl.size() == 0) {
        if (!diskMap) {
            LOGERROR << "Scavenger cannot create disk scavengers without a disk map";
            return ERR_NOT_READY;
        }

        DiskIdSet hddIds = diskMap->getDiskIds(diskio::diskTier);
        DiskIdSet ssdIds = diskMap->getDiskIds(diskio::flashTier);
        // create scavengers for HDDs
        for (DiskIdSet::const_iterator cit = hddIds.cbegin();
             cit != hddIds.cend();
             ++cit) {
            DiskScavenger *diskScav = new DiskScavenger(*cit, diskio::diskTier,
                                                        dataStoreReqHandler,
                                                        persistStoreGcHandler,
                                                        diskMap,
                                                        noPersistScavStats);
            fds_verify(diskScavTbl.count(*cit) == 0);
            diskScavTbl[*cit] = diskScav;
            LOGNORMAL << "Added scavenger for HDD " << *cit;
        }
        // create scavengers for SSDs
        for (DiskIdSet::const_iterator cit = ssdIds.cbegin();
             cit != ssdIds.cend();
             ++cit) {
            DiskScavenger *diskScav = new DiskScavenger(*cit, diskio::flashTier,
                                                        dataStoreReqHandler,
                                                        persistStoreGcHandler,
                                                        diskMap,
                                                        noPersistScavStats);
            fds_verify(diskScavTbl.count(*cit) == 0);
            diskScavTbl[*cit] = diskScav;
            LOGNORMAL << "Added scavenger for SSD " << *cit;
        }
    }
    noPersistScavStats = false;
    return ERR_OK;
}

Error
ScavControl::updateDiskScavenger(const SmDiskMap::const_ptr& diskMap,
                                 const DiskId& diskId,
                                 const bool& added) {
    if (!diskMap) {
        LOGERROR << "Scavenger cannot create disk scavenger(s) without a disk map";
        return ERR_NOT_READY;
    }

    fds_mutex::scoped_lock l(scav_lock);
    if (added) {
        DiskScavenger *diskScav = new DiskScavenger(diskId,
                                                    diskMap->diskMediaType(diskId),
                                                    dataStoreReqHandler,
                                                    persistStoreGcHandler,
                                                    diskMap,
                                                    noPersistScavStats);
        fds_assert(diskScavTbl.count(diskId) == 0);
        if (diskScavTbl.count(diskId) != 0) {
            LOGERROR << "Scavenger already exists for disk=" << diskId;
            delete diskScav;
            return ERR_DUPLICATE;
        }
        diskScavTbl[diskId] = diskScav;
        LOGNORMAL << "Added scavenger for Disk " << diskId;
    } else {
        DiskScavenger *diskScav = diskScavTbl[diskId];
        if (diskScav) {
            diskScav->handleScavengeError(ERR_SM_NO_DISK);
            LOGNORMAL << "Disabled scavenger for disk=" << diskId;
        }
    }

    if (diskScavTbl.size()) {
        noPersistScavStats = false;
    }

    return ERR_OK;
}

Error
ScavControl::updateDiskScavengers(const SmDiskMap::const_ptr& diskMap,
                                  const DiskIdSet& diskIdSet,
                                  const bool& added) {
    Error err(ERR_OK);
    if (!diskMap) {
        LOGERROR << "Scavenger cannot create disk scavenger(s) without a disk map";
        return ERR_NOT_READY;
    }

    for (auto& diskId : diskIdSet) {
        err = updateDiskScavenger(diskMap, diskId, added);
    }
    return err;
}

Error ScavControl::enableScavenger(const SmDiskMap::const_ptr& diskMap,
                                   SmCommandInitiator initiator)
{
    Error err(ERR_OK);
    fds_bool_t expect = false;

    // if this is called for the first time and diskScavTbl is empty,
    // populate diskScavTable first (before changing enabled to true,
    // so that the scavengers do not start to early)
    err = createDiskScavengers(diskMap);
    if (!err.ok()) {
        LOGERROR << "Failed to enable Scavenger " << err;
        return err;
    }

    // check whether we can enable scavenger based on who disabled it
    fds_bool_t initiatorUser = (initiator == SM_CMD_INITIATOR_USER);
    if ((std::atomic_load(&enabled) == false) &&
        (whoDisabledMe != SM_CMD_INITIATOR_NOT_SET)) {
        // if user tries to enable scavenger and it is temporary
        // disabled by the background process, just report an error
        if (initiatorUser && (whoDisabledMe != SM_CMD_INITIATOR_USER)) {
            LOGNOTIFY << "Scavenger is temporary disabled by the background process "
                      << whoDisabledMe << ", try again later...";
            return ERR_SM_GC_TEMP_DISABLED;
        }
        // if background process tries to re-enable scavenger which is
        // disabled by the user, just ignore
        if (!initiatorUser && (whoDisabledMe == SM_CMD_INITIATOR_USER)) {
            LOGNORMAL << "Scavenger is disabled by the user, background processs "
                      << " cannot re-enable it; Scavenger will remain disabled";
            return ERR_OK;
        }
    }

    if (!std::atomic_compare_exchange_strong(&enabled, &expect, true)) {
        LOGNOTIFY << "Scavenger cycle is already enabled, nothing else to do";
        return ERR_SM_GC_ENABLED;
    }
    LOGNOTIFY << "Enabling Scavenger";

    // start timer to update disk stats
    if (!scav_timer->scheduleRepeated(scav_timer_task,
                                      std::chrono::seconds(intervalSeconds))) {
        LOGWARN << "Failed to schedule timer for updating disks stats! "
                << " will not run automatic GC, only manual";
        return ERR_SM_AUTO_GC_FAILED;
    }

    return err;
}

void ScavControl::disableScavenger(SmCommandInitiator initiator)
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
        whoDisabledMe = initiator;
    } else {
        // if someone tried to disable scavenger that was not yet initialized,
        // remember who, so that we follow the right procedure for enabling it
        if (whoDisabledMe == SM_CMD_INITIATOR_NOT_SET) {
            whoDisabledMe = initiator;
        }
        LOGNOTIFY << "Scavenger was already disabled, saved initiator " << initiator;
    }

    // Disable scrubber also
    if (std::atomic_compare_exchange_strong(&verifyData, &expect, false)) {
        LOGNOTIFY << "Disabled scrubber because scavenger was disabled";
    } else {
        LOGDEBUG << "Scrubber status not changed; scrubber already disabled";
    }
}

void
ScavControl::setPersistState() {
    // call this method in disabled state only
    fds_verify(!std::atomic_load(&enabled));
    noPersistScavStats = true;
}

void ScavControl::startScavengeProcess()
{
    fds_bool_t _enabled = std::atomic_load(&enabled);
    if (!_enabled) {
        LOGNOTIFY << "Cannot start Scavenger process because scavenger"
                  << " is disabled";
        return;
    }

    bool periodic_expunge = g_fdsprocess->get_fds_config()->\
            get<bool>("fds.feature_toggle.common.periodic_expunge", false);
    if (periodic_expunge &&
        !(dynamic_cast<ObjectStorMgr*>(dataStoreReqHandler)->haveAllObjectSets()))
    {
        LOGNOTIFY << "Object sets not present for all the volumes."
                  << "Cannot start scavenger";
        return;
    }

    LOGNORMAL << "Scavenger cycle - Started";
    fds_mutex::scoped_lock l(scav_lock);
    // start first max_disks_compacting disk scavengers
    fds_uint32_t count = 0;
    if (nextDiskToCompact != SM_INVALID_DISK_ID) {
        LOGWARN << "Looks like GC is already in progress.. not going to start GC ";
        return;
    }

    OBJECTSTOREMGR(dataStoreReqHandler)->counters->scavengerStartedAt.set(util::getTimeStampSeconds());
    OBJECTSTOREMGR(dataStoreReqHandler)->counters->dataCopied.set(0);
    OBJECTSTOREMGR(dataStoreReqHandler)->counters->dataRemoved.set(0);

    for (DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
         cit != diskScavTbl.cend();
         ++cit) {
        DiskScavenger *diskScav = cit->second;
        if (count >= max_disks_compacting) {
            nextDiskToCompact = diskScav->diskId();
            ++count;  // so that we can check if we updated next disk ID
            break;
        }
        if (diskScav != NULL) {
            Error err = diskScav->startScavenge(verifyData, std::bind(
                &ScavControl::diskCompactionDoneCb, this,
                std::placeholders::_1, std::placeholders::_2));
            if (err.ok()) {
                ++count;
            }
        }
    }
    if (count <= max_disks_compacting) {
        nextDiskToCompact = SM_INVALID_DISK_ID;
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
    // Stop GC on all the disks
    for (DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
         cit != diskScavTbl.cend();
         ++cit) {
        DiskScavenger *diskScav = cit->second;
        if (diskScav != NULL) {
            diskScav->stopScavenge();
        }
    }
}

void
ScavControl::diskCompactionDoneCb(fds_uint16_t diskId, const Error& error) {
    fds_mutex::scoped_lock l(scav_lock);
    OBJECTSTOREMGR(dataStoreReqHandler)->counters->scavengerRunning.decr();
    if (nextDiskToCompact == SM_INVALID_DISK_ID) {
        LOGDEBUG << "No more disks to compact";
        bool allDone = true;
        for (const auto& scav : diskScavTbl) {
            allDone = allDone && (scav.second->getState() == DiskScavenger::SCAV_STATE_IDLE);
            if (!allDone) break;
        }
        if (allDone) {
            LOGNORMAL << "Scavenger cycle - Done";
        }
        return;
    }
    DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
    // find disk scavenger that is reponsible for disk ID nextDiskToCompact
    while ((cit != diskScavTbl.cend()) && ((cit->second)->diskId() != nextDiskToCompact)) {
        ++cit;
    }
    Error err(ERR_NOT_FOUND);
    while (cit != diskScavTbl.cend()) {
        DiskScavenger *diskScav = cit->second;
        if (diskScav) {
            err = diskScav->startScavenge(verifyData, std::bind(
                &ScavControl::diskCompactionDoneCb, this,
                std::placeholders::_1, std::placeholders::_2));
        }
        ++cit;
        if (err.ok()) {
            // started compaction of the next disk
            if (cit != diskScavTbl.cend()) {
                nextDiskToCompact = (cit->second)->diskId();
            } else {
                err = ERR_NOT_FOUND;
            }
            break;
        }
    }
    if (!err.ok()) {
        // did not find next scavenger to start
        nextDiskToCompact = SM_INVALID_DISK_ID;
    }
}

// set policy on all disks
Error
ScavControl::setScavengerPolicy(fds_uint32_t dsk_avail_threshold_1,
                                fds_uint32_t dsk_avail_threshold_2,
                                fds_uint32_t tok_reclaim_threshold,
                                fds_uint32_t proc_max_tokens) {
    Error err(ERR_OK);
    DiskScavPolicyInternal policy;
    if ((dsk_avail_threshold_1 > 100) ||
        (dsk_avail_threshold_2 > 100) ||
        (tok_reclaim_threshold > 100)) {
        return ERR_INVALID_ARG;
    }
    // params are valid
    policy.dsk_avail_threshold_1 = dsk_avail_threshold_1;
    policy.dsk_avail_threshold_2 = dsk_avail_threshold_2;
    policy.tok_reclaim_threshold = tok_reclaim_threshold;
    policy.proc_max_tokens = proc_max_tokens;

    // set same policy for all disks, the policy will take effect
    // in the next GC cycle (not the current one if it is currently in
    // progress), except for proc_max_tokens which will take effect
    // in the current GC cycle
    LOGNORMAL << "Changing scavenger policy for every disk to " << policy;
    fds_mutex::scoped_lock l(scav_lock);
    for (DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
         cit != diskScavTbl.cend();
         ++cit) {
        DiskScavenger *diskScav = cit->second;
        if (diskScav != NULL) {
            diskScav->setPolicy(policy);
        }
    }

    return err;
}

// returns policy (for now we have same policy on all disks
// so getting from first disk we find)
Error
ScavControl::getScavengerPolicy(const fpi::CtrlQueryScavengerPolicyRespPtr& policyResp) {
    Error err(ERR_OK);
    DiskScavenger *diskScav = NULL;
    {  // for getting scavenger policy
        fds_mutex::scoped_lock l(scav_lock);
        DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
        if (cit == diskScavTbl.cend()) {
            return ERR_NOT_FOUND;  // no disks
        }
        diskScav = cit->second;
    }
    fds_verify(diskScav != NULL);
    DiskScavPolicyInternal policy = diskScav->getPolicy();

    policyResp->dsk_threshold1 = policy.dsk_avail_threshold_1;
    policyResp->dsk_threshold2 = policy.dsk_avail_threshold_2;
    policyResp->token_reclaim_threshold = policy.tok_reclaim_threshold;
    policyResp->tokens_per_dsk = policy.proc_max_tokens;
    LOGNORMAL << "Scavenger policy: " << policy;
    return err;
}

void
ScavControl::getScavengerStatus(const fpi::CtrlQueryScavengerStatusRespPtr& statusResp) {
    if (std::atomic_load(&enabled) == false) {
        statusResp->status = fpi::SCAV_DISABLED;
        LOGNORMAL << "Scavenger is disabled";
        return;
    }
    // scavenger is active if at least one disk is active
    // and scavenger is stopping if at least one disk is stopping
    // since for now all disk scavenger are started and stopped at
    // the same time...
    fds_mutex::scoped_lock l(scav_lock);
    for (DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
         cit != diskScavTbl.cend();
         ++cit) {
        DiskScavenger *diskScav = cit->second;
        if (diskScav != NULL) {
            DiskScavenger::ScavState state = diskScav->getState();
            if (state == DiskScavenger::SCAV_STATE_INPROG) {
                statusResp->status = fpi::SCAV_ACTIVE;
                return;
            } else if (state == DiskScavenger::SCAV_STATE_STOPPING) {
                statusResp->status = fpi::SCAV_STOPPING;
                return;
            }
        }
    }
    statusResp->status = fpi::SCAV_INACTIVE;
}

void
ScavControl::getDataVerify(const fpi::CtrlQueryScrubberStatusRespPtr& statusResp) {
    // First get scavenger status
    GLOGDEBUG << "Calling getDataVerify";
    boost::shared_ptr<fpi::CtrlQueryScavengerStatusResp> scavStatus =
            boost::shared_ptr<fpi::CtrlQueryScavengerStatusResp>
                    (new fpi::CtrlQueryScavengerStatusResp());
    getScavengerStatus(scavStatus);

    if (std::atomic_load(&verifyData) == true) {
        statusResp->scrubber_status = scavStatus->status;
    } else {
        statusResp->scrubber_status = fpi::SCAV_DISABLED;
    }
    GLOGDEBUG << "Set statusResp->" << statusResp->scrubber_status;
}


fds_uint32_t
ScavControl::getProgress() {
    double totalToksCompacting = 0;
    double totalToksFinished = 0;
    double progress = 0;

    if (std::atomic_load(&enabled) == false) {
        return 100;
    }

    fds_mutex::scoped_lock l(scav_lock);
    for (DiskScavTblType::const_iterator cit = diskScavTbl.cbegin();
         cit != diskScavTbl.cend();
         ++cit) {
        fds_uint32_t toksCompacting = 0;
        fds_uint32_t toksFinished = 0;
        DiskScavenger *diskScav = cit->second;
        if (diskScav != NULL) {
            diskScav->getProgress(&toksCompacting,
                                  &toksFinished);
            totalToksCompacting += toksCompacting;
            totalToksFinished += toksFinished;
        }
    }

    fds_verify(totalToksFinished <= totalToksCompacting);
    LOGTRACE << "total compacting " << totalToksCompacting
             << " toks finished " << totalToksFinished;
    if (totalToksCompacting > 0) {
        progress = 100 * totalToksFinished / totalToksCompacting;
    }
    return progress;
}

DiskScavenger::DiskScavenger(fds_uint16_t _disk_id,
                             diskio::DataTier _tier,
                             SmIoReqHandler *data_store,
                             SmPersistStoreHandler* persist_store,
                             const SmDiskMap::const_ptr& diskMap,
                             fds_bool_t noPersistStateScavStats)
        : disk_id(_disk_id),
          disk_scav_lock("Disk-Scav Lock"),
          smDiskMap(diskMap),
          persistStoreGcHandler(persist_store),
          done_evt_handler(nullptr),
          tier(_tier),
          dataStoreReqHandler(data_store),
          noPersistScavStats(noPersistStateScavStats),
          scav_policy()  // default policy
{
    state = ATOMIC_VAR_INIT(SCAV_STATE_IDLE);
    next_token = 0;

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

DiskScavPolicyInternal
DiskScavenger::getPolicy() const {
    return scav_policy;
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
    (*retStat).dsk_tot_size = statbuf.f_blocks * statbuf.f_frsize;
    (*retStat).dsk_avail_size = statbuf.f_bfree * statbuf.f_bsize;
    (*retStat).dsk_reclaim_size = totDeletedBytes;
    return err;
}

fds_bool_t DiskScavenger::updateDiskStats(fds_bool_t verify_data,
                                          disk_compaction_done_handler_t done_hdlr) {
    Error err(ERR_OK);
    DiskStat disk_stat;
    double tot_size;
    fds_uint32_t avail_percent;  // percent of available capacity
    fds_uint32_t token_reclaim_threshold = 0;
    verifyData = verify_data;

    err = getDiskStats(&disk_stat);
    if (!err.ok()) {
        LOGCRITICAL << "Getting disk stats failed for disk id : "
                    << disk_id;
        return false;
    }

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
        err = startScavenge(verifyData, done_hdlr, token_reclaim_threshold);
        return err.ok();
    }

    return false;
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
    ObjectStorMgr* storMgr = dynamic_cast<ObjectStorMgr*>(dataStoreReqHandler);

    // add tokens to tokenDb that we need to compact
    for (SmTokenSet::const_iterator cit = diskToks.cbegin();
         cit != diskToks.cend();
         ++cit) {
        diskio::TokenStat stat;

        if (g_fdsprocess->get_fds_config()->\
            get<bool>("fds.feature_toggle.common.periodic_expunge", false)){
            /**
             * Evaluate all bloom filters for this particular SM token.
             * and calculate SM token threshold.
             */

            bool fHasNewObjects = true;
            if (storMgr) {
                fHasNewObjects = storMgr->objectStore->liveObjectsTable->hasNewObjectSets(*cit, disk_id);
            }

            if (!fHasNewObjects) {
                LOGDEBUG << "no new object sets to process for disk:" << disk_id << " token:"<< *cit;
            } else {
                if (storMgr) {
                    TimeStamp now = util::getTimeStampSeconds() * 1000 * 1000 * 1000;
                    storMgr->objectStore->liveObjectsTable->setTokenStartTime(*cit, disk_id, now);
                }
                persistStoreGcHandler->evaluateSMTokenObjSets(*cit, tier, stat);
            }
        } else {
            persistStoreGcHandler->getSmTokenStats(*cit, tier, &stat);
        }
        double tot_size = stat.tkn_tot_size;
        reclaim_percent = (stat.tkn_reclaim_size / tot_size) * 100;
        OBJECTSTOREMGR(dataStoreReqHandler)->counters->setScavengeInfo(stat.tkn_id,
                                                                       stat.tkn_tot_size,
                                                                       stat.tkn_reclaim_size
                                                                       );
        LOGDEBUG << "Disk " << disk_id << " token " << stat.tkn_id
                 << " total bytes " << stat.tkn_tot_size
                 << ", deleted bytes " << stat.tkn_reclaim_size
                 << " (" << reclaim_percent << "%)";

        if (stat.tkn_reclaim_size > 0 &&
            reclaim_percent >= token_reclaim_threshold) {
                tokenDb.insert(stat.tkn_id);
                LOGNOTIFY << "TC will run for token " << stat.tkn_id
                          << "Disk " << disk_id
                          << " total bytes " << stat.tkn_tot_size
                          << ", deleted bytes " << stat.tkn_reclaim_size
                          << " (" << reclaim_percent << "%)";
        }
    }
}

Error DiskScavenger::startScavenge(fds_bool_t verify,
                                   disk_compaction_done_handler_t done_hdlr,
                                   fds_uint32_t token_reclaim_threshold) {
    Error err(ERR_OK);
    fds_uint32_t i = 0;
    fds_token_id tok_id;
    ScavState expectState = SCAV_STATE_IDLE;
    if (!std::atomic_compare_exchange_strong(&state, &expectState, SCAV_STATE_INPROG)) {
        LOGNOTIFY << "Scavenger is either running or trying to finish, ignoring command";
        return ERR_NOT_READY;
    }

    // type of work we are going to do
    verifyData = verify;
    done_evt_handler = done_hdlr;
    OBJECTSTOREMGR(dataStoreReqHandler)->counters->scavengerRunning.incr();
    // get list of tokens for this tier/disk from persistent layer
    findTokensToCompact(token_reclaim_threshold);
    if (tokenDb.size() == 0) {
        LOGDEBUG << "no tokens to compact on disk:" << disk_id << " tier:" << tier;
        std::atomic_exchange(&state, SCAV_STATE_IDLE);
        OBJECTSTOREMGR(dataStoreReqHandler)->counters->scavengerRunning.decr();
        return ERR_NOT_FOUND;
     }

    LOGNORMAL << "scavenger started for disk:" << disk_id << " tier:"
              << tier << " num.tokens:" << tokenDb.size()
              << " assume no scavenger stats = " << noPersistScavStats;
    next_token = 0;

    for (i = 0; i < scav_policy.proc_max_tokens; ++i) {
        fds_bool_t found = getNextCompactToken(&tok_id);
        if (!found) {
            LOGNORMAL << "scavenger finished for disk:" << disk_id;
            std::atomic_exchange(&state, SCAV_STATE_IDLE);
            break;
        }
        tok_compactor_vec[i]->startCompaction(tok_id, disk_id, tier, verifyData, std::bind(
               &DiskScavenger::compactionDoneCb, this,
               std::placeholders::_1, std::placeholders::_2));
    }

    return err;
}

void DiskScavenger::stopScavenge() {
    ScavState expectState = SCAV_STATE_INPROG;
    OBJECTSTOREMGR(dataStoreReqHandler)->counters->scavengerRunning.decr();
    if (std::atomic_compare_exchange_strong(&state, &expectState, SCAV_STATE_STOPPING)) {
        LOGNOTIFY << "Stopping scavenger cycle...";
    } else {
        LOGNOTIFY << "Scavenger was not running (or already in the process of stopping) "
                  << "for disk:" << disk_id;
    }
}

void
DiskScavenger::handleScavengeError(const Error& err) {
    std::atomic_exchange(&state, SCAV_STATE_ERROR);
    LOGNOTIFY << "scavenger for disk:" << disk_id << " in error:" << err;
}

void
DiskScavenger::getProgress(fds_uint32_t *toksCompacting,
                           fds_uint32_t *toksFinished) {
    *toksCompacting = 0;
    *toksFinished = 0;
    if (atomic_load(&state) == SCAV_STATE_IDLE) {
        LOGNORMAL << "No progress, because disk scavenger is idle";
        return;
    }

    // we already got scavenger lock, and this map changes
    fds_uint32_t count = 0;
    fds_uint32_t nextTok = next_token;
    // TODO(Anna) make next_token atomic,.. ok here, because we do not
    // need to be super exact in progress reporting
    for (std::set<fds_token_id>::const_iterator cit = tokenDb.cbegin();
         cit != tokenDb.cend();
         ++cit) {
        if (*cit < nextTok) {
            ++count;
        } else {
            break;
        }
    }
    *toksCompacting = tokenDb.size();
    *toksFinished = count;
    LOGNORMAL << "disk:" << disk_id << " progress: " << *toksCompacting
              << " total tokens compacting, " << *toksFinished
              << " total tokens finished compaction";
}

void DiskScavenger::compactionDoneCb(fds_token_id token_id, const Error& error) {
    fds_token_id tok_id;
    ScavState curState = std::atomic_load(&state);

    LOGDEBUG  << "compaction done for token:" << token_id
              << " disk:" << disk_id << " verify:" << verifyData
              << " result:" << error;

    fds_verify(curState != SCAV_STATE_IDLE);
    OBJECTSTOREMGR(dataStoreReqHandler)->counters->compactorRunning.decr();
    if (curState == SCAV_STATE_STOPPING) {
        // Scavenger was asked to stop, so not compacting any more tokens
        std::atomic_store(&state, SCAV_STATE_IDLE);
        LOGNOTIFY << "Not continuing with token compaction, because stop was called";
        return;
    }

    // find available token compactor and start compaction of next token
    // even if the current token compaction error'ed we should move forward
    // compacting other tokens in the disk.
    for (fds_uint32_t i = 0; i < scav_policy.proc_max_tokens; ++i) {
        if (tok_compactor_vec[i]->isIdle()) {
            if (!getNextCompactToken(&tok_id)) {
                break;
            }
            tok_compactor_vec[i]->startCompaction(tok_id, disk_id, tier, verifyData,
                                 std::bind(
                                     &DiskScavenger::compactionDoneCb, this,
                                     std::placeholders::_1, std::placeholders::_2));
         }
    }

    fds_bool_t finished = true;
    for (fds_uint32_t i = 0; i < scav_policy.proc_max_tokens; ++i) {
        finished = (finished && tok_compactor_vec[i]->isIdle());
        if (!finished) break;
    }

    if (finished) {
        noPersistScavStats = false;
        ScavState expectState = SCAV_STATE_INPROG;
        if (std::atomic_compare_exchange_strong(&state, &expectState, SCAV_STATE_IDLE)) {
            LOGNORMAL << "Scavenger process finished for disk: " << disk_id;
            if (done_evt_handler) {
                done_evt_handler(disk_id, Error(ERR_OK));
            }
            done_evt_handler = nullptr;
        } else {
            expectState = SCAV_STATE_STOPPING;
            if (std::atomic_compare_exchange_strong(&state, &expectState, SCAV_STATE_IDLE)) {
                LOGNORMAL << "Scavenger was stopped, but scavenging process completed for "
                          << "disk:" << disk_id;
            }
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

std::ostream& operator<< (std::ostream &out,
                          const DiskScavPolicyInternal& policy) {
    out << "Disk threshold 1 = " << policy.dsk_avail_threshold_1 << ",";
    out << "Disk threshold 2 = " << policy.dsk_avail_threshold_2 << ",";
    out << "Threshold for reclaimable bytes in SM token = "
        << policy.tok_reclaim_threshold << ",";
    out << "Max tokens to compact per disk = " << policy.proc_max_tokens;
    return out;
}

}  // namespace fds

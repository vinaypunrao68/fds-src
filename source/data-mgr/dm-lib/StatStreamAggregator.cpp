/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <limits>
#include <vector>
#include <fds_process.h>
#include <net/net-service.h>
#include <util/math-util.h>
#include <DataMgr.h>
#include <StatStreamAggregator.h>
#include <fdsp/Streaming.h>

namespace fds {

class VolStatsTimerTask : public FdsTimerTask {
  public:
    VolumeStats* volstats_;

    VolStatsTimerTask(FdsTimer &timer,
                      VolumeStats* volstats)
            : FdsTimerTask(timer)
    {
        volstats_ = volstats;
    }
    ~VolStatsTimerTask() {}

    virtual void runTimerTask() override;
};

VolumeStats::VolumeStats(fds_volid_t volume_id,
                         fds_uint64_t start_time,
                         fds_uint32_t finestat_slots,
                         fds_uint32_t finestat_slotsec)
        : volid_(volume_id),
          finegrain_hist_(new VolumePerfHistory(volume_id,
                                                start_time,
                                                finestat_slots,
                                                finestat_slotsec)),
          coarsegrain_hist_(new VolumePerfHistory(volume_id,
                                                  start_time,
                                                  5,  // 24 1-hour slots + slot
                                                  5*60)),  // 1-hour, but temp 5-min
          longterm_hist_(new VolumePerfHistory(volume_id,
                                               start_time,
                                               3,  // 30 1-day slots + slot
                                               10*60)),  // 1 day, but temp 1 h
          long_stdev_update_ts_(0),
          recent_stdev_update_ts_(0),
          cap_recent_stdev_(0),
          cap_long_stdev_(0),
          cap_recent_wma_(0),
          perf_recent_stdev_(0),
          perf_long_stdev_(0),
          perf_recent_wma_(0),
          process_tm_(new FdsTimer()),
          process_tm_task_(new VolStatsTimerTask(*process_tm_, this)) {
    // this is how often we process 1-min stats
    // setting short interval for now, so we can see logging more often
    // for debugging/demo;
    fds_uint32_t max_tmperiod_sec = (finestat_slots - 1) * finestat_slotsec;
    tmperiod_sec_ = 60 * 5;  // every 5 min
    if (tmperiod_sec_ > max_tmperiod_sec) {
        tmperiod_sec_ = max_tmperiod_sec;
    }
    last_print_ts_ = 0;

    // start the timer
    fds_bool_t ret = process_tm_->scheduleRepeated(process_tm_task_,
                                                   std::chrono::seconds(tmperiod_sec_));
    if (!ret) {
        LOGERROR << "Failed to schedule timer for logging volume stats!";
    }
}

VolumeStats::~VolumeStats() {
    process_tm_->destroy();
}

void VolumeStats::processStats() {
    LOGDEBUG << "Will log 1-min stats to file";
    std::vector<StatSlot> slots;

    // note that we are passing time interval modules push stats to the aggregator
    // -- this is time interval back from current time that will be not included into
    // the list of slots returned, because these slots may not be fully filled yet, eg.
    // aggregator will receive stats from some modules but maybe not from all modules yet.
    last_print_ts_ = finegrain_hist_->toSlotList(slots, last_print_ts_, 0, FdsStatPushPeriodSec);
    // first add the slots to coarse grain stat history (we do it here rather than on
    // receiving remote stats because it's more efficient
    if (slots.size() > 0) {
        coarsegrain_hist_->mergeSlots(slots,
                                      finegrain_hist_->getStartTime());
        LOGDEBUG << "Updated coarse-grain stats: " << *coarsegrain_hist_;
        longterm_hist_->mergeSlots(slots,
                                   finegrain_hist_->getStartTime());
        LOGDEBUG << "Updated long-term stats: " << *longterm_hist_;
    }

    for (std::vector<StatSlot>::const_iterator cit = slots.cbegin();
         cit != slots.cend();
         ++cit) {
        // TODO(Anna) add slot to 1 hour history
        // debugging for now, remove when we start printing this to file
        LOGNOTIFY << "[" << finegrain_hist_->getTimestamp((*cit).getTimestamp()) << "], "
                  << "Volume " << std::hex << volid_ << std::dec << ", "
                  << "Puts " << StatHelper::getTotalPuts(*cit) << ", "
                  << "Gets " << StatHelper::getTotalGets(*cit) << ", "
                  << "Queue full " << StatHelper::getQueueFull(*cit) << ", "
                  << "Logical bytes " << StatHelper::getTotalLogicalBytes(*cit) << ", "
                  << "Physical bytes " << StatHelper::getTotalPhysicalBytes(*cit) << ", "
                  << "Metadata bytes " << StatHelper::getTotalMetadataBytes(*cit) << ", "
                  << "Blobs " << StatHelper::getTotalBlobs(*cit) << ", "
                  << "Objects " << StatHelper::getTotalObjects(*cit) << ", "
                  << "Ave blob size " << StatHelper::getAverageBytesInBlob(*cit) << " bytes, "
                  << "Ave objects in blobs " << StatHelper::getAverageObjectsInBlob(*cit) << ", "
                  << "% of ssd accesses " << StatHelper::getPercentSsdAccesses(*cit) << ", "
                  << "Short term perf wma " << perf_recent_wma_ << ", "
                  << "Short term perf sigma " << perf_recent_stdev_ << ", "
                  << "Long term perf sigma " << perf_long_stdev_ << ", "
                  << "Short term capacity sigma " << cap_recent_stdev_ << ", "
                  << "Short term capacity wma " << cap_recent_wma_ << ", "
                  << "Long term capacity sigma " << cap_long_stdev_;
    }

    // log to 1-minute file
    // if enough time passed, also log to 1hour file
}


//
// returns firebreak related metrics; may update cached metrics
// first if they are not recent enough
//
void VolumeStats::getFirebreakMetrics(double* recent_cap_stdev,
                                      double* long_cap_stdev,
                                      double* recent_cap_wma,
                                      double* recent_perf_stdev,
                                      double* long_perf_stdev,
                                      double* recent_perf_wma) {
    // update cached metrics if they are stale
    updateFirebreakMetrics();
    *recent_cap_stdev = cap_recent_stdev_;
    *long_cap_stdev = cap_long_stdev_;
    *recent_cap_wma = cap_recent_wma_;
    *recent_perf_stdev = perf_recent_stdev_;
    *long_perf_stdev = perf_long_stdev_;
    *recent_perf_wma = perf_recent_wma_;
}

//
// calculates stdev for performance and capacity
//
void VolumeStats::updateFirebreakMetrics() {
    std::vector<StatSlot> slots;
    fds_uint64_t now = util::getTimeStampNanos();
    fds_uint64_t elapsed_nanos = 0;
    if (now > long_stdev_update_ts_) {
        elapsed_nanos = now - long_stdev_update_ts_;
    }
    // check if it's time to update recent history stdev
    if (elapsed_nanos < (NANOS_IN_SECOND * coarsegrain_hist_->secondsInSlot())) {
        return;  // no need to update long-term stdev
    }
    recent_stdev_update_ts_ = now;

    // update recent (one day / 1h slots) history stdev
    coarsegrain_hist_->toSlotList(slots, 0, 0, FdsStatPushPeriodSec);
    if (slots.size() < (coarsegrain_hist_->numberOfSlots() - 1)) {
        // we must have mostly full history to start recording stdev
        return;
    }
    updateStdev(slots, 1, &cap_recent_stdev_, &perf_recent_stdev_,
                &cap_recent_wma_, &perf_recent_wma_);
    LOGDEBUG << "Short term history size " << slots.size()
             << " seconds in slot " << coarsegrain_hist_->secondsInSlot()
             << " capacity wma " << cap_recent_wma_
             << " perf wma " << perf_recent_wma_
             << " capacity stdev " << cap_recent_stdev_
             << " perf stdev " << perf_recent_stdev_;

    // check if it's time to update long term history stdev
    if (elapsed_nanos < (NANOS_IN_SECOND * longterm_hist_->secondsInSlot())) {
        return;  // no need to update long-term stdev
    }
    long_stdev_update_ts_ = now;

    // get 1-day history
    longterm_hist_->toSlotList(slots, 0, 0, FdsStatPushPeriodSec);
    if (slots.size() < (longterm_hist_->numberOfSlots() - 1)) {
        // to start reporting long term stdev we must have most of history
        return;
    }
    double units = longterm_hist_->secondsInSlot() / coarsegrain_hist_->secondsInSlot();
    updateStdev(slots, 1, &cap_long_stdev_, &perf_long_stdev_, NULL, NULL);
    LOGDEBUG << "Long term history size " << slots.size()
             << " seconds in slot " << coarsegrain_hist_->secondsInSlot()
             << " short slot units " << units
             << " capacity stdev " << cap_long_stdev_
             << " perf stdev " << perf_long_stdev_;
}

void VolumeStats::updateStdev(const std::vector<StatSlot>& slots,
                              double units_in_slot,
                              double* cap_stdev,
                              double* perf_stdev,
                              double* cap_wma,
                              double* perf_wma) {
    std::vector<double> add_bytes;
    std::vector<double> ops_vec;
    double prev_bytes = 0;
    for (std::vector<StatSlot>::const_iterator cit = slots.cbegin();
         cit != slots.cend();
         ++cit) {
        double bytes = StatHelper::getTotalPhysicalBytes(*cit);
        if (cit == slots.cbegin()) {
            prev_bytes = bytes;
            continue;
        }
        double add_bytes_per_unit = (bytes - prev_bytes) / units_in_slot;
        double ops = StatHelper::getTotalPuts(*cit) + StatHelper::getTotalGets(*cit);
        double ops_per_unit = ops / units_in_slot;

        LOGDEBUG << "Volume " << std::hex << volid_ << std::dec << " rel ts "
                 << (*cit).getTimestamp() << " bytes/unit added " << (bytes - prev_bytes)
                 << " ops/unit " << ops;

        add_bytes.push_back(add_bytes_per_unit);
        ops_vec.push_back(ops_per_unit);
    }

    // get weighted rolling average of coarse-grain (1-h) histories
    *cap_stdev = util::fds_get_wstdev(add_bytes);
    *perf_stdev = util::fds_get_wstdev(ops_vec);
    if (cap_wma) {
        double wsum = 0;
        *cap_wma = util::fds_get_wma(add_bytes, &wsum);
    }
    if (perf_wma) {
        double wsum = 0;
        *perf_wma = util::fds_get_wma(ops_vec, &wsum);
    }
}


StatStreamAggregator::StatStreamAggregator(char const *const name)
        : Module(name),
          finestat_slotsec_(FdsStatPeriodSec),
          finestat_slots_(65) {
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    // align timestamp to the length of finestat slot
    start_time_ = util::getTimeStampNanos();
    fds_uint64_t freq_nanos = finestat_slotsec_ * 1000000000;
    start_time_ = start_time_ / freq_nanos;
    start_time_ = start_time_ * freq_nanos;

    root->fds_mkdir(root->dir_user_repo_stats().c_str());

    fpi::StatStreamRegistrationMsgPtr minLogReg(new fpi::StatStreamRegistrationMsg());
    minLogReg->id = std::numeric_limits<fds_uint32_t>::max();
    minLogReg->method = "log-local";
    minLogReg->sample_freq_seconds = 60;
    minLogReg->duration_seconds = 900;

    fpi::StatStreamRegistrationMsgPtr hourLogReg(new fpi::StatStreamRegistrationMsg());
    hourLogReg->id = std::numeric_limits<fds_uint32_t>::max() - 1;
    hourLogReg->method = "log-local";
    hourLogReg->sample_freq_seconds = 3600;
    hourLogReg->duration_seconds = 7200;

    registerStream(minLogReg);
    registerStream(hourLogReg);
}

StatStreamAggregator::~StatStreamAggregator() {
}

int StatStreamAggregator::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

void StatStreamAggregator::mod_startup()
{
}

void StatStreamAggregator::mod_shutdown()
{
}

Error StatStreamAggregator::attachVolume(fds_volid_t volume_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Will monitor stats for vol " << std::hex
             << volume_id << std::dec;

    // create Volume Stats struct and add it to the map
    read_synchronized(volstats_lock_) {
        volstats_map_t::iterator it  = volstats_map_.find(volume_id);
        if (volstats_map_.end() != it) {
            fds_assert(it->second);
            // for now return an error, but prob it's ok if we
            // try to attach volume we already keeping stats for
            // just continue using them?
            return ERR_VOL_DUPLICATE;
        }
    }
    // actually create volume stats struct
    VolumeStats::ptr new_hist(new VolumeStats(volume_id,
                                              start_time_,
                                              finestat_slots_,
                                              finestat_slotsec_));
    write_synchronized(volstats_lock_) {
        if (volstats_map_.count(volume_id) == 0) {
            volstats_map_[volume_id] = new_hist;
        }
    }

    return err;
}

VolumeStats::ptr StatStreamAggregator::getVolumeStats(fds_volid_t volid) {
    read_synchronized(volstats_lock_) {
        volstats_map_t::iterator it  = volstats_map_.find(volid);
        if (volstats_map_.end() != it) {
            fds_assert(it->second);
            return it->second;
        }
    }
    return VolumeStats::ptr();
}

Error StatStreamAggregator::detachVolume(fds_volid_t volume_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Will stop monitoring stats for vol " << std::hex
             << volume_id << std::dec;

    // remove volstats struct from the map, the destructor should
    // close log file, etc.
    write_synchronized(volstats_lock_) {
        if (volstats_map_.count(volume_id) > 0) {
            volstats_map_.erase(volume_id);
        }
    }
    return err;
}

Error StatStreamAggregator::registerStream(fpi::StatStreamRegistrationMsgPtr registration) {
    if (!registration->sample_freq_seconds || !registration->duration_seconds) {
        return ERR_INVALID_ARG;
    } else if (registration->sample_freq_seconds != 60 &&
            registration->sample_freq_seconds != 3600) {
        return ERR_INVALID_ARG;
    } else if (registration->duration_seconds % registration->sample_freq_seconds) {
        return ERR_INVALID_ARG;
    }

    LOGDEBUG << "Adding streaming registration with id " << registration->id;

    SCOPEDWRITE(lockStatStreamRegsMap);
    statStreamRegistrations_[registration->id] = registration;
    FdsTimerTaskPtr task(new StatStreamTimerTask(timer_, registration, *this));
    statStreamTaskMap_[registration->id] = task;
    timer_.scheduleRepeated(task, std::chrono::seconds(registration->duration_seconds));
    return ERR_OK;
}

Error StatStreamAggregator::deregisterStream(fds_uint32_t reg_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Removing streaming registration with id " << reg_id;

    SCOPEDWRITE(lockStatStreamRegsMap);
    timer_.cancel(statStreamTaskMap_[reg_id]);
    statStreamTaskMap_.erase(reg_id);
    statStreamRegistrations_.erase(reg_id);
    return err;
}

Error
StatStreamAggregator::handleModuleStatStream(const fpi::StatStreamMsgPtr& stream_msg) {
    Error err(ERR_OK);
    fds_uint64_t remote_start_ts = stream_msg->start_timestamp;
    for (fds_uint32_t i = 0; i < (stream_msg->volstats).size(); ++i) {
        fpi::VolStatList & vstats = (stream_msg->volstats)[i];
        LOGDEBUG << "Received stats for volume " << std::hex << vstats.volume_id
                 << std::dec << " timestamp " << remote_start_ts;
        VolumeStats::ptr volstats = getVolumeStats(vstats.volume_id);
        if (!volstats) {
          LOGWARN << "Volume " << std::hex << vstats.volume_id << std::dec
               << " is not attached to the aggregator! Ignoring stats";
          continue;
        }
        err = (volstats->finegrain_hist_)->mergeSlots(vstats, remote_start_ts);
        fds_verify(err.ok());  // if err, prob de-serialize issue
        LOGDEBUG << *(volstats->finegrain_hist_);
    }
    return err;
}

Error
StatStreamAggregator::writeStatsLog(const fpi::volumeDataPoints& volStatData,
                                                            fds_volid_t vol_id,
                                                            bool isMin /* = true */) {
    Error err(ERR_OK);
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    const std::string fileName = root->dir_user_repo_stats() + std::to_string(vol_id) +
            std::string("/") + (isMin ? "stat_min.log" : "stat_hour.log");

    FILE   *pFile = fopen((const char *)fileName.c_str(), "a+");
    if (!pFile) {
        GLOGWARN << "Error opening stat log file '" << fileName << "'";
        return ERR_NOT_FOUND;
    }

    fprintf(pFile, "%s,", volStatData.volume_name.c_str());
    fprintf(pFile, "%ld,", volStatData.timestamp);

    std::vector<DataPointPair>::const_iterator pos;

    fprintf(pFile, "[");
    for (pos = volStatData.meta_list.begin(); pos != volStatData.meta_list.end(); ++pos)
        fprintf(pFile, " %s: %f,", pos->key.c_str(), pos->value);
    fprintf(pFile, "]");

    fprintf(pFile, "\n");
    fclose(pFile);
    return err;
}

Error
StatStreamAggregator::getStatStreamRegDetails(const fds_volid_t & volId,
        fpi::SvcUuid & amId, fds_uint32_t & regId) {
    SCOPEDREAD(lockStatStreamRegsMap);
    for (auto entry : statStreamRegistrations_) {
        for (auto vol : entry.second->volumes) {
            if (volId == vol) {
                amId = entry.second->dest;
                regId = entry.second->id;
                return ERR_OK;
            }
        }
    }

    return ERR_NOT_FOUND;
}

//
// merge local stats for a given volume
//
void
StatStreamAggregator::handleModuleStatStream(fds_uint64_t start_timestamp,
                                             fds_volid_t volume_id,
                                             const std::vector<StatSlot>& slots) {
    fds_uint64_t remote_start_ts = start_timestamp;

    LOGDEBUG << "Received stats for volume " << std::hex << volume_id
             << std::dec << " timestamp " << remote_start_ts;

    VolumeStats::ptr volstats = getVolumeStats(volume_id);
    if (!volstats) {
        LOGWARN << "Volume " << std::hex << volume_id << std::dec
           << " is not attached to the aggregator! Ignoring stats";
        return;
    }
    (volstats->finegrain_hist_)->mergeSlots(slots, remote_start_ts);
    LOGDEBUG << *(volstats->finegrain_hist_);
}

fds_uint64_t StatHelper::getTotalPuts(const StatSlot& slot) {
    return slot.getCount(STAT_AM_PUT_OBJ);
}

fds_uint64_t StatHelper::getTotalGets(const StatSlot& slot) {
    return slot.getCount(STAT_AM_GET_OBJ);
}

fds_uint64_t StatHelper::getTotalLogicalBytes(const StatSlot& slot) {
    return slot.getTotal(STAT_DM_CUR_TOTAL_BYTES);
}

fds_uint64_t StatHelper::getTotalPhysicalBytes(const StatSlot& slot) {
    fds_uint64_t logical_bytes = slot.getTotal(STAT_DM_CUR_TOTAL_BYTES);
    fds_uint64_t deduped_bytes =  slot.getTotal(STAT_SM_CUR_DEDUP_BYTES);
    if (logical_bytes < deduped_bytes) {
        // we did not measure something properly, for now ignoring
        LOGWARN << "logical bytes " << logical_bytes << " < deduped bytes "
                << deduped_bytes;
        return 0;
    }
    return (logical_bytes - deduped_bytes);
}

// we do not include user-defined metadata
fds_uint64_t StatHelper::getTotalMetadataBytes(const StatSlot& slot) {
    fds_uint64_t tot_objs = slot.getTotal(STAT_DM_CUR_TOTAL_OBJECTS);
    fds_uint64_t tot_blobs = slot.getTotal(STAT_DM_CUR_TOTAL_BLOBS);
    // approx -- asume 20 bytes for blob name
    fds_uint64_t header_bytes = 20 + 8*3 + 4;
    return (tot_blobs * header_bytes + tot_objs * 24);
}

fds_uint64_t StatHelper::getTotalBlobs(const StatSlot& slot) {
    return slot.getTotal(STAT_DM_CUR_TOTAL_BLOBS);
}

fds_uint64_t StatHelper::getTotalObjects(const StatSlot& slot) {
    return slot.getTotal(STAT_DM_CUR_TOTAL_OBJECTS);
}

double StatHelper::getAverageBytesInBlob(const StatSlot& slot) {
    double tot_bytes = slot.getTotal(STAT_DM_CUR_TOTAL_BYTES);
    double tot_blobs = slot.getTotal(STAT_DM_CUR_TOTAL_BLOBS);
    if (tot_blobs == 0) return 0;
    return tot_bytes / tot_blobs;
}

double StatHelper::getAverageObjectsInBlob(const StatSlot& slot) {
    double tot_objects = slot.getTotal(STAT_DM_CUR_TOTAL_OBJECTS);
    double tot_blobs = slot.getTotal(STAT_DM_CUR_TOTAL_BLOBS);
    if (tot_blobs == 0) return 0;
    return tot_objects / tot_blobs;
}

double StatHelper::getPercentSsdAccesses(const StatSlot& slot) {
    double ssd_ops = slot.getCount(STAT_SM_OP_SSD);
    double hdd_ops = slot.getCount(STAT_SM_OP_HDD);
    if (hdd_ops == 0) return 0;
    return (100.0 * ssd_ops / (ssd_ops + hdd_ops));
}

fds_bool_t StatHelper::getQueueFull(const StatSlot& slot) {
    return (slot.getMin(STAT_AM_QUEUE_FULL) > 4);  // random constant
}

void VolStatsTimerTask::runTimerTask() {
    volstats_->processStats();
}

void StatStreamTimerTask::runTimerTask() {
    LOGDEBUG << "Streaming stats for registration '" << reg_->id << "'";

    std::vector<fpi::volumeDataPoints> dataPoints;

    std::vector<fds_volid_t> volumes;
    if (reg_->volumes.empty()) {
        for (auto i : statStreamAggr_.volstats_map_) {
            volumes.push_back(i.first);
        }
    } else {
        for (auto i : reg_->volumes) {
            volumes.push_back(i);
        }
    }

    bool logLocal = reg_->method == std::string("log-local");
    for (auto volId : volumes) {
        std::vector<StatSlot> slots;
        std::unordered_map<fds_uint64_t, std::vector<fpi::DataPointPair> > volDataPointsMap;
        VolumeStats::ptr volStat = statStreamAggr_.getVolumeStats(volId);
        if (!volStat) {
            GLOGWARN << "Cannot get stat volume history for id '" << volId << "'";
            continue;
        }
        const std::string & volName = dataMgr->volumeName(volId);

        VolumePerfHistory::ptr & hist = 60 == reg_->sample_freq_seconds ?
                volStat->finegrain_hist_ : volStat->coarsegrain_hist_;
        last_ts_ = hist->toSlotList(slots, last_ts_, 0, FdsStatPushPeriodSec);

        for (auto slot : slots) {
            fds_uint64_t timestamp = hist->getTimestamp((slot).getTimestamp());

            timestamp /= 1000 * 1000 * 1000;

            fpi::DataPointPair putsDP;
            putsDP.key = "Puts";
            putsDP.value = StatHelper::getTotalPuts(slot);
            volDataPointsMap[timestamp].push_back(putsDP);

            fpi::DataPointPair getsDP;
            getsDP.key = "Gets";
            getsDP.value = StatHelper::getTotalGets(slot);
            volDataPointsMap[timestamp].push_back(getsDP);

            fpi::DataPointPair logicalBytesDP;
            logicalBytesDP.key = "Logical Bytes";
            logicalBytesDP.value = StatHelper::getTotalLogicalBytes(slot);
            volDataPointsMap[timestamp].push_back(logicalBytesDP);

            fpi::DataPointPair blobsDP;
            blobsDP.key = "Blobs";
            blobsDP.value = StatHelper::getTotalBlobs(slot);
            volDataPointsMap[timestamp].push_back(blobsDP);

            fpi::DataPointPair objectsDP;
            objectsDP.key = "Objects";
            objectsDP.value = StatHelper::getTotalObjects(slot);
            volDataPointsMap[timestamp].push_back(objectsDP);

            fpi::DataPointPair aveBlobSizeDP;
            aveBlobSizeDP.key = "Ave Blob Size";
            aveBlobSizeDP.value = StatHelper::getAverageBytesInBlob(slot);
            volDataPointsMap[timestamp].push_back(aveBlobSizeDP);

            fpi::DataPointPair aveObjectsDP;
            aveObjectsDP.key = "Ave Objects per Blob";
            aveObjectsDP.value = StatHelper::getAverageObjectsInBlob(slot);
            volDataPointsMap[timestamp].push_back(aveObjectsDP);

            fpi::DataPointPair percentSSDAccessDP;
            percentSSDAccessDP.key = "Percent of SSD Accesses";
            percentSSDAccessDP.value = StatHelper::getPercentSsdAccesses(slot);
            volDataPointsMap[timestamp].push_back(percentSSDAccessDP);

            fpi::DataPointPair recentCapStdevDP;
            recentCapStdevDP.key = "Short term capacity sigma";

            fpi::DataPointPair longCapStdevDP;
            longCapStdevDP.key = "Long term capacity sigma";

            fpi::DataPointPair recentCapWmaDP;
            recentCapWmaDP.key = "Short term capacity wma";

            fpi::DataPointPair recentPerfStdevDP;
            recentPerfStdevDP.key = "hort term perf sigma";

            fpi::DataPointPair longPerfStdevDP;
            longPerfStdevDP.key = "Long term perf sigma";

            fpi::DataPointPair recentPerfWma;
            recentPerfWma.key = "Short term perf wma";

            volStat->getFirebreakMetrics(&recentCapStdevDP.value, &longCapStdevDP.value,
                    &recentCapWmaDP.value, &recentPerfStdevDP.value, &longPerfStdevDP.value,
                    &recentPerfWma.value);

            volDataPointsMap[timestamp].push_back(recentCapStdevDP);
            volDataPointsMap[timestamp].push_back(longCapStdevDP);
            volDataPointsMap[timestamp].push_back(recentCapWmaDP);
            volDataPointsMap[timestamp].push_back(recentPerfStdevDP);
            volDataPointsMap[timestamp].push_back(longPerfStdevDP);
            volDataPointsMap[timestamp].push_back(recentPerfWma);
        }

        for (auto dp : volDataPointsMap) {
            fpi::volumeDataPoints volDataPoint;
            volDataPoint.volume_name = volName;
            volDataPoint.timestamp = dp.first;
            volDataPoint.meta_list = dp.second;

            if (logLocal) {
                statStreamAggr_.writeStatsLog(volDataPoint, volId,
                        60 == reg_->sample_freq_seconds);
            }
            dataPoints.push_back(volDataPoint);
        }
    }

    if (dataPoints.size() == 0) {
        // nothing to send, no attached volumes
        return;
    }

    if (!logLocal) {
        EpInvokeRpc(fpi::StreamingClient, publishMetaStream,
                reg_->dest, 0, 2, reg_->id, dataPoints);
    }
}
}  // namespace fds

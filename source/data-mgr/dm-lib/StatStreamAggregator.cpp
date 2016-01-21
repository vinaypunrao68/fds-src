/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <limits>
#include <vector>
#include <fdsp/fds_stream_types.h>
#include <fds_process.h>
#include <net/SvcMgr.h>
#include <util/math-util.h>
#include <DataMgr.h>
#include <StatStreamAggregator.h>
#include <fdsp/Streaming.h>
#include <time.h>

namespace fds {

#define MinLogRegId (std::numeric_limits<fds_uint32_t>::max())
#define HourLogRegId (std::numeric_limits<fds_uint32_t>::max() - 1)

class VolStatsTimerTask : public FdsTimerTask {
  public:
    StatStreamAggregator* aggr_;

    VolStatsTimerTask(FdsTimer &timer,
                      StatStreamAggregator* aggr)
            : FdsTimerTask(timer)
    {
        aggr_ = aggr;
    }
    ~VolStatsTimerTask() {}

    virtual void runTimerTask() override;
};

StatStreamTimerTask::StatStreamTimerTask(FdsTimer &timer,
                                         fpi::StatStreamRegistrationMsgPtr reg,
                                         StatStreamAggregator & statStreamAggr,
                                         DataMgr& dataManager)
        : FdsTimerTask(timer),
          dataManager_(dataManager),
          reg_(reg),
          statStreamAggr_(statStreamAggr) {
    for (auto volId : reg_->volumes) {
        vol_last_rel_sec_[fds_volid_t(volId)] = std::numeric_limits<fds_uint64_t>::max(); // Indicates unused.
    }
}

void StatStreamTimerTask::cleanupVolumes(const std::vector<fds_volid_t>& cur_vols) {
    std::vector<fds_volid_t> rm_vols;
    for (auto i : vol_last_rel_sec_) {
        fds_bool_t found = false;
        for (auto volId : cur_vols) {
            if (i.first == volId) {
                found = true;
                break;
            }
        }
        if (!found) rm_vols.push_back(i.first);
    }
    for (auto rmvol : rm_vols) {
        vol_last_rel_sec_.erase(rmvol);
    }
}

VolumeStats::VolumeStats(fds_volid_t volume_id,
                         fds_uint64_t start_time,
                         const StatHistoryConfig& conf)
        : volid_(volume_id),
          finegrain_hist_(new VolumePerfHistory(volume_id,
                                                start_time,
                                                conf.finestat_slots_,
                                                conf.finestat_slotsec_)),
          coarsegrain_hist_(new VolumePerfHistory(volume_id,
                                                  start_time,
                                                  conf.coarsestat_slots_,
                                                  conf.coarsestat_slotsec_)),
          longterm_hist_(new VolumePerfHistory(volume_id,
                                               start_time,
                                               conf.longstat_slots_,
                                               conf.longstat_slotsec_)),
          cap_recent_stdev_(0),
          cap_long_stdev_(0),
          cap_recent_wma_(0),
          perf_recent_stdev_(0),
          perf_long_stdev_(0),
          perf_recent_wma_(0),
          last_process_rel_secs_(0) {
}

VolumeStats::~VolumeStats() {
}

void VolumeStats::processStats() {
    /**
     * Get latest, unprocessed fine-grained slots to merge with our coarse grained slots.
     */
    std::vector<StatSlot> slots;
    last_process_rel_secs_ = finegrain_hist_->toSlotList(slots,
                                                         last_process_rel_secs_,
                                                         StatConstants::singleton()->FdsStatCollectionWaitMult * StatConstants::singleton()->FdsStatPushAndAggregatePeriodSec);
                                                            // the last few slots that are contained in the amount of time taken to
                                                            // do FdsStatCollectionWaitMult pushes.
                                                            // Why ignore those last few? Well, we are assuming that not all modules feeding
                                                            // their stats to us are keeping up and we are willing to wait up to
                                                            // FdsStatCollectionWaitMult push periods for them to catch up.

    // first add the slots to coarse grain stat history (we do it here rather than on
    // receiving remote stats because it's more efficient)
    if (slots.size() > 0) {
        coarsegrain_hist_->mergeSlots(slots);
        LOGTRACE << "Updated coarse-grain stats: " << *coarsegrain_hist_;
        longterm_hist_->mergeSlots(slots);
        LOGTRACE << "Updated long-term stats: " << *longterm_hist_;
    } else {
        LOGTRACE << "No qualifying fine-grained slots to merge into coarse-grained. last_process_rel_secs_ is <"
                 << last_process_rel_secs_ << ">.";
    }

    //for (std::vector<StatSlot>::const_iterator cit = slots.cbegin();
    //     cit != slots.cend();
    //     ++cit) {
    //    // debugging for now, remove at some point
    //    LOGTRACE << "[" << (*cit).getRelSeconds() << "], "
    //             << "Volume " << std::hex << volid_ << std::dec << ", "
    //             << "Puts " << StatHelper::getCountObjPuts(*cit) << ", "
    //             << "Gets " << StatHelper::getTotalObjGets(*cit) << ", "
    //             << "Queue full " << StatHelper::getQueueFull(*cit) << ", "
    //             << "Logical bytes " << StatHelper::getTotalLogicalBytes(*cit) << ", "
    //             << "Physical bytes " << StatHelper::getTotalPhysicalBytes(*cit) << ", "
    //             << "Metadata bytes " << StatHelper::getTotalMetadataBytes(*cit) << ", "
    //             << "Blobs " << StatHelper::getTotalBlobs(*cit) << ", "
    //             << "Objects " << StatHelper::getTotalLogicalObjects(*cit) << ", "
    //             << "Ave blob size " << StatHelper::getAverageBytesInBlob(*cit) << " bytes, "
    //             << "Ave objects in blobs " << StatHelper::getAverageObjectsInBlob(*cit) << ", "
    //             << "Gets from SSD " << StatHelper::getTotalSsdGets(*cit) << ", "
    //             << "Gets from HDD " <<
    //             StatHelper::getTotalObjGets(*cit) - StatHelper::getTotalSsdGets(*cit) << ", "
    //             << "Short term perf wma " << perf_recent_wma_ << ", "
    //             << "Short term perf sigma " << perf_recent_stdev_ << ", "
    //             << "Long term perf sigma " << perf_long_stdev_ << ", "
    //             << "Short term capacity sigma " << cap_recent_stdev_ << ", "
    //             << "Short term capacity wma " << cap_recent_wma_ << ", "
    //             << "Long term capacity sigma " << cap_long_stdev_;
    //}
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

    // update recent (coarse-grained slots) history stdev
    coarsegrain_hist_->toSlotList(slots, 0);

    updateStdev(slots, 1, &cap_recent_stdev_, &perf_recent_stdev_,
                &cap_recent_wma_, &perf_recent_wma_);
    LOGTRACE << "Short term history size " << slots.size()
             << " seconds in slot " << coarsegrain_hist_->secondsInSlot()
             << " capacity wma " << cap_recent_wma_
             << " perf wma " << perf_recent_wma_
             << " capacity stdev " << cap_recent_stdev_
             << " perf stdev " << perf_recent_stdev_;

    // get long-term history
    longterm_hist_->toSlotList(slots, 0);
    double units = longterm_hist_->secondsInSlot() / coarsegrain_hist_->secondsInSlot();
    updateStdev(slots, 1, &cap_long_stdev_, &perf_long_stdev_, NULL, NULL);
    LOGTRACE << "Long term history size " << slots.size()
             << " seconds in slot " << longterm_hist_->secondsInSlot()
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
         cit++) {
        double bytes = StatHelper::getTotalPhysicalBytes(*cit);
        LOGTRACE << "1 - Volume " << std::hex << volid_ << std::dec << " rel secs "
                 << (*cit).getRelSeconds() << " bytes " << static_cast<fds_int64_t>(bytes);

        if (cit == slots.cbegin()) {
            prev_bytes = bytes;
            continue;
        }
        double add_bytes_per_unit = (bytes - prev_bytes) / units_in_slot;
        double ops = StatHelper::getCountObjPuts(*cit) + StatHelper::getCountObjGets(*cit);
        double ops_per_unit = ops / units_in_slot;

        LOGTRACE << "Volume " << std::hex << volid_ << std::dec << " rel secs "
                 << (*cit).getRelSeconds() << " bytes " << static_cast<fds_int64_t>(bytes)
                 << " prev bytes " << static_cast<fds_int64_t>(prev_bytes)
                 << " add_bytes_per_unit " << static_cast<fds_int64_t>(add_bytes_per_unit)
                 << " ops " << ops << " ops/unit " << ops_per_unit;

        add_bytes.push_back(add_bytes_per_unit);
        ops_vec.push_back(ops_per_unit);
        prev_bytes = bytes;
    }

    // get weighted rolling average of coarse-grain histories
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


StatStreamAggregator::StatStreamAggregator(CommonModuleProviderIf *modProvider,
                                           char const *const name,
                                           boost::shared_ptr<FdsConfig> fds_config,
                                           DataMgr& dataManager)
        : HasModuleProvider(modProvider),
          Module(name),
          dataManager_(dataManager),
          aggregate_stats_(MODULEPROVIDER()->getTimer()),
          aggregate_stats_task_(new VolStatsTimerTask(*aggregate_stats_, this)) {
    const FdsRootDir *root = MODULEPROVIDER()->proc_fdsroot();

    // default config
    hist_config.finestat_slotsec_ = StatConstants::singleton()->FdsStatFGPeriodSec;
    hist_config.finestat_slots_ = StatConstants::singleton()->FdsStatFGSlotCnt;
    hist_config.coarsestat_slotsec_ = StatConstants::singleton()->FdsStatCGPeriodSec;
    hist_config.coarsestat_slots_ = StatConstants::singleton()->FdsStatCGSlotCnt;
    hist_config.longstat_slotsec_ = StatConstants::singleton()->FdsStatLTPeriodSec;
    hist_config.longstat_slots_ = StatConstants::singleton()->FdsStatLTSlotCnt;

    // if in firebreak test mode, set different slot sizes for long and coarse
    // grain histories
    fds_bool_t test_firebreak = fds_config->get<bool>("fds.dm.testing.test_firebreak", false);
    if (test_firebreak) {
        hist_config.coarsestat_slotsec_ = fds_config->\
                get<int>("fds.dm.testing.coarseslot_sec", hist_config.coarsestat_slotsec_);
        hist_config.coarsestat_slots_ = fds_config->\
                get<int>("fds.dm.testing.coarse_slots", hist_config.coarsestat_slots_);
        hist_config.longstat_slotsec_ = fds_config->\
                get<int>("fds.dm.testing.longslot_sec", hist_config.longstat_slotsec_);
        hist_config.longstat_slots_ = fds_config->\
                get<int>("fds.dm.testing.long_slots", hist_config.longstat_slots_);
    }

    /**
     * Make sure our fine-grained stats are not collected less frequently than
     * our coarse grained stats.
     */
    if (hist_config.coarsestat_slotsec_ < hist_config.finestat_slotsec_) {
        LOGWARN << "Changing fine-grained stat period, currently at <" << hist_config.finestat_slotsec_
                << ">, to be at least the same period as our coarse-grained stats which is <"
                << hist_config.coarsestat_slotsec_ << ">.";
        hist_config.finestat_slotsec_ = hist_config.coarsestat_slotsec_;
    }

    // align timestamp to the length of finestat slot
    start_time_ = util::getTimeStampNanos();
    fds_uint64_t freq_nanos = hist_config.finestat_slotsec_ * NANOS_IN_SECOND;
    start_time_ = start_time_ / freq_nanos;
    start_time_ = start_time_ * freq_nanos;

    root->fds_mkdir(root->dir_sys_repo_stats().c_str());

    /**
     * This sets up our local logging of stats - local with respect to the stats
     * collection primary DM for a volume and only for volume-context stats.
     */
    fpi::StatStreamRegistrationMsgPtr minLogReg(new fpi::StatStreamRegistrationMsg());
    minLogReg->id = MinLogRegId;
    minLogReg->method = "log-local";
    minLogReg->sample_freq_seconds = StatConstants::singleton()->FdsStatFGStreamPeriodFactorSec*1; // Smallest allowed streaming period.
    minLogReg->duration_seconds = StatConstants::singleton()->FdsStatRunForever;

    fpi::StatStreamRegistrationMsgPtr hourLogReg(new fpi::StatStreamRegistrationMsg());
    hourLogReg->id = HourLogRegId;
    hourLogReg->method = "log-local";
    fds_assert(StatConstants::singleton()->FdsStatFGStreamPeriodFactorSec*30 == 3600);  // We want this to be an hour and it has to be a multiple of StatConstants::singleton()->FdsStatFGStreamPeriodFactorSec.
    hourLogReg->sample_freq_seconds = StatConstants::singleton()->FdsStatFGStreamPeriodFactorSec*30;
    hourLogReg->duration_seconds = StatConstants::singleton()->FdsStatRunForever;

    registerStream(minLogReg);
    registerStream(hourLogReg);

    /**
     * This sets up our internal aggregation of fine-grained stats into coarse-grained stats.
     */
    LOGTRACE << "Finegrain slot period " << hist_config.finestat_slotsec_ << " seconds, "
             << "slots " << hist_config.finestat_slots_ << "; coarsegrain slot period "
             << hist_config.coarsestat_slotsec_ << " seconds, slots " << hist_config.coarsestat_slots_
             << ". Will update coarse grain stats every " << StatConstants::singleton()->FdsStatPushAndAggregatePeriodSec << " seconds.";

    fds_bool_t ret = aggregate_stats_->scheduleRepeated(aggregate_stats_task_,
                                                        std::chrono::seconds(StatConstants::singleton()->FdsStatPushAndAggregatePeriodSec));

    if (!ret) {
        LOGTRACE << "Failed to schedule timer for logging volume stats!";
    }
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
    LOGTRACE << "Will monitor stats for vol " << std::hex
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
                                              hist_config));
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
    LOGTRACE << "Will stop monitoring stats for vol " << std::hex
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
    } else if (registration->sample_freq_seconds % StatConstants::singleton()->FdsStatFGStreamPeriodFactorSec != 0) {
        return ERR_INVALID_ARG;
    } else if ((registration->duration_seconds % registration->sample_freq_seconds) &&
                (registration->duration_seconds != StatConstants::singleton()->FdsStatRunForever)) {
        return ERR_INVALID_ARG;
    }

    LOGTRACE << "Adding streaming registration with id <" << registration->id
             << "> and a stream period of <" << registration->sample_freq_seconds << "> seconds.";

    /**
     * Schedule the task to repeatedly stream stats at the expiration of our interval.
     */
    SCOPEDWRITE(lockStatStreamRegsMap);
    statStreamRegistrations_[registration->id] = registration;
    FdsTimerTaskPtr statStreamTask(new StatStreamTimerTask(streamTimer_, registration, *this, dataManager_));
    statStreamTaskMap_[registration->id] = statStreamTask;
    streamTimer_.scheduleRepeated(statStreamTask, std::chrono::seconds(registration->sample_freq_seconds));

    /**
     * Schedule the task to cancel our stream stats task at the expiration of our duration unless
     * we are asked to run the task indefinitely.
     */
    if (registration->duration_seconds != StatConstants::singleton()->FdsStatRunForever) {
        LOGTRACE << "Streaming registration with id <" << registration->id << "> to be canceled after <"
                        << registration->duration_seconds << "> seconds.";
        FdsTimerTaskPtr statStreamCancelTask(
                new StatStreamCancelTimerTask(cancelTimer_, streamTimer_, registration->id, statStreamTask));
        cancelTimer_.schedule(statStreamCancelTask, std::chrono::seconds(registration->duration_seconds));
    }

    return ERR_OK;
}

Error StatStreamAggregator::deregisterStream(fds_uint32_t reg_id) {
    Error err(ERR_OK);
    LOGTRACE << "Removing streaming registration with id " << reg_id;

    SCOPEDWRITE(lockStatStreamRegsMap);
    streamTimer_.cancel(statStreamTaskMap_[reg_id]);
    statStreamTaskMap_.erase(reg_id);
    statStreamRegistrations_.erase(reg_id);
    return err;
}

Error
StatStreamAggregator::handleModuleStatStream(const fpi::StatStreamMsgPtr& stream_msg) {
    Error err(ERR_OK);

    for (fds_uint32_t i = 0; i < (stream_msg->volstats).size(); ++i) {
        fpi::VolStatList & vstats = (stream_msg->volstats)[i];
        //LOGTRACE << "Received " << vstats.statlist.size() << " generations of fdsp stats for volume " << std::hex << vstats.volume_id
        //         << std::dec << " start timestamp " << stream_msg->start_timestamp
        //         << "\n" << VolStatListWrapper(vstats);

        VolumeStats::ptr volstats = getVolumeStats(fds_volid_t(vstats.volume_id));
        if (!volstats) {
          LOGTRACE << "Volume " << fds_volid_t(vstats.volume_id)
                   << " is not attached to the aggregator! Ignoring stats.";
          continue;
        }

        err = (volstats->finegrain_hist_)->mergeSlots(vstats, stream_msg->start_timestamp);
        fds_verify(err.ok());  // if err, prob de-serialize issue
    }
    return err;
}

//
// merge local stats for a given volume
//
void
StatStreamAggregator::handleModuleStatStream(fds_uint64_t start_timestamp,
                                             fds_volid_t volume_id,
                                             const std::vector<StatSlot>& slots) {
    //LOGTRACE << "Received local stats for volume " << std::hex << volume_id
    //         << std::dec << " start timestamp " << start_timestamp;

    VolumeStats::ptr volstats = getVolumeStats(volume_id);
    if (!volstats) {
        LOGTRACE << "Volume " << std::hex << volume_id << std::dec
                 << " is not attached to the aggregator! Ignoring stats.";
        return;
    }
    (volstats->finegrain_hist_)->mergeSlots(slots);
}

Error
StatStreamAggregator::writeStatsLog(const fpi::volumeDataPoints& volStatData,
                                                            fds_volid_t vol_id,
                                                            bool isMin /* = true */) {
    Error err(ERR_OK);
    char buf[50];

    const FdsRootDir* root = MODULEPROVIDER()->proc_fdsroot();
    const std::string fileName = root->dir_sys_repo_stats() + std::to_string(vol_id.get()) +
            std::string("/") + (isMin ? "stat_min.log" : "stat_hour.log");

    FILE   *pFile = fopen((const char *)fileName.c_str(), "a+");
    if (!pFile) {
        GLOGTRACE << "Error opening stat log file '" << fileName << "'";
        return ERR_NOT_FOUND;
    }

    fprintf(pFile, "%s,", volStatData.volume_name.c_str());
    ctime_r((&volStatData.timestamp), buf);
    *(strchr(buf, '\n')) = '\0';  // Get rid of the \n at the end
    fprintf(pFile, "%s,", buf);

    std::vector<fpi::DataPointPair>::const_iterator pos;

    fprintf(pFile, "[");
    for (pos = volStatData.meta_list.begin(); pos != volStatData.meta_list.end(); ++pos) {
        // FIXME: Add range checking.
        fprintf(pFile, " %s: %lld,", pos->key.c_str(), static_cast<long long>(pos->value));
    }
    fprintf(pFile, "]");

    fprintf(pFile, "\n");
    fclose(pFile);

    /* rsync the per volume stats */
    if (dataManager_.amIPrimary(vol_id)) {
        DmtColumnPtr nodes = MODULEPROVIDER()->getSvcMgr()->getDMTNodesForVolume(vol_id);
        fds_verify(nodes->getLength() > 0);

        auto selfSvcUuid = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid();
        for (fds_uint32_t i = 0; i < nodes->getLength(); i++) {
            //LOGTRACE << " rsync node id: " << (nodes->get(i)).uuid_get_val()
            //                   << "myuuid: " << selfSvcUuid.svc_uuid;
           if (selfSvcUuid != nodes->get(i).toSvcUuid()) {
               /* Disable volStatSync because we shouldn't be using rsync to sync stats
                * One reaason is the below code use system() command to run rsync
                * When you use system command it does a fork and copies all fds.  This
                * causes issues when the parent restarts and child still owns fds opened
                * by previous incarnation of the parent
                */
              // volStatSync(nodes->get(i).uuid_get_val(), vol_id);
           }
        }
    }
    return err;
}


Error
StatStreamAggregator::volStatSync(NodeUuid dm_uuid, fds_volid_t vol_id) {
    Error err(ERR_OK);
    const FdsRootDir* root = MODULEPROVIDER()->proc_fdsroot();
    const std::string src_dir = root->dir_sys_repo_stats() + std::to_string(vol_id.get()) +
                                              std::string("/");
    const fpi::SvcUuid & dmSvcUuid = dm_uuid.toSvcUuid();
    auto svcmgr = MODULEPROVIDER()->getSvcMgr();

    fpi::SvcInfo dmSvcInfo;
    if (!svcmgr->getSvcInfo(dmSvcUuid, dmSvcInfo)) {
        LOGTRACE << "Failed to sync vol stat: Failed to get IP address for destination DM "
                << std::hex << dmSvcUuid.svc_uuid << std::dec;
        return ERR_NOT_FOUND;
    }

    std::string node_root = svcmgr->getSvcProperty(
        svcmgr->mapToSvcUuid(dmSvcUuid, fpi::FDSP_PLATFORM), "fds_root");
    std::string dst_ip = dmSvcInfo.ip;

    const std::string dst_node = node_root + "sys-repo/vol-stats/" +
                                    std::to_string(vol_id.get()) + std::string("/");
    const std::string rsync_cmd = "sshpass -p passwd rsync -r "
            + src_dir + "  root@" + dst_ip + ":" + dst_node + "";

    LOGTRACE << "system rsync: " <<  rsync_cmd;

    int retcode = std::system((const char *)rsync_cmd.c_str());
    return err;
}


Error
StatStreamAggregator::getStatStreamRegDetails(const fds_volid_t & volId,
        fpi::SvcUuid & amId, fds_uint32_t & regId) {
    SCOPEDREAD(lockStatStreamRegsMap);
    for (auto entry : statStreamRegistrations_) {
        for (auto vol : entry.second->volumes) {
            if (volId == fds_volid_t(vol)) {
                amId = entry.second->dest;
                regId = entry.second->id;
                return ERR_OK;
            }
        }
    }

    return ERR_NOT_FOUND;
}

//
// called on timer to update coarse grain and long term history for
// volume stats
//
void StatStreamAggregator::processStats() {
    std::vector<fds_volid_t> volumes;
    read_synchronized(volstats_lock_) {
        for (auto i : volstats_map_) {
            volumes.push_back(i.first);
        }
    }
    for (auto volId : volumes) {
        VolumeStats::ptr volStat = getVolumeStats(volId);

        /**
         * Perhaps the volume was removed between the time we saw it in
         * building this copy of the VolStats map and when we try to
         * get the VolStats here.
         */
        if (volStat) {
            volStat->processStats();
        }
    }
}

fds_uint64_t StatHelper::getCountObjPuts(const StatSlot &slot) {
    return slot.getCount(STAT_AM_PUT_OBJ);
}

fds_uint64_t StatHelper::getCountExplicitTxBlobPuts(const StatSlot &slot) {
    return slot.getCount(STAT_AM_PUT_BLOB);
}

fds_uint64_t StatHelper::getCountImplicitTxBlobPuts(const StatSlot &slot) {
    return slot.getCount(STAT_AM_PUT_BLOB_ONCE);
}

fds_uint64_t StatHelper::getCountObjGets(const StatSlot &slot) {
    return slot.getCount(STAT_AM_GET_OBJ);
}

fds_uint64_t StatHelper::getCountCachedObjGets(const StatSlot &slot) {
    return slot.getCount(STAT_AM_GET_CACHED_OBJ);
}

fds_uint64_t StatHelper::getCountSSDObjGets(const StatSlot &slot) {
    return slot.getCount(STAT_SM_GET_SSD);
}

fds_uint64_t StatHelper::getCountBlobGets(const StatSlot &slot) {
    return slot.getCount(STAT_AM_GET_BLOB);
}

fds_uint64_t StatHelper::getCountAttachVol(const StatSlot &slot) {
    return slot.getCount(STAT_AM_ATTACH_VOL);
}

fds_uint64_t StatHelper::getCountDetachVol(const StatSlot &slot) {
    return slot.getCount(STAT_AM_DETACH_VOL);
}

fds_uint64_t StatHelper::getCountVolStat(const StatSlot &slot) {
    return slot.getCount(STAT_AM_VSTAT);
}

fds_uint64_t StatHelper::getCountPutVolMeta(const StatSlot &slot) {
    return slot.getCount(STAT_AM_PUT_VMETA);
}

fds_uint64_t StatHelper::getCountGetVolMeta(const StatSlot &slot) {
    return slot.getCount(STAT_AM_GET_VMETA);
}

fds_uint64_t StatHelper::getCountGetVolContents(const StatSlot &slot) {
    return slot.getCount(STAT_AM_GET_VCONTENTS);
}

fds_uint64_t StatHelper::getCountStartBlobTx(const StatSlot &slot) {
    return slot.getCount(STAT_AM_START_BLOB_TX);
}

fds_uint64_t StatHelper::getCountCommitBlobTx(const StatSlot &slot) {
    return slot.getCount(STAT_AM_COMMIT_BLOB_TX);
}

fds_uint64_t StatHelper::getCountAbortBlobTx(const StatSlot &slot) {
    return slot.getCount(STAT_AM_ABORT_BLOB_TX);
}

fds_uint64_t StatHelper::getCountDeleteBlob(const StatSlot &slot) {
    return slot.getCount(STAT_AM_DELETE_BLOB);
}

fds_uint64_t StatHelper::getCountRenameBlob(const StatSlot &slot) {
    return slot.getCount(STAT_AM_RENAME_BLOB);
}

fds_uint64_t StatHelper::getTotalLogicalBytes(const StatSlot& slot) {
    return slot.getTotal(STAT_DM_CUR_LBYTES);
}

fds_uint64_t StatHelper::getTotalPhysicalBytes(const StatSlot& slot) {
    /**
     * We used to calculate physical bytes thusly:
     */
    //fds_uint64_t logical_bytes = slot.getTotal(STAT_DM_CUR_LBYTES);
    //fds_uint64_t deduped_bytes =  slot.getTotal(STAT_SM_CUR_DOMAIN_DEDUP_BYTES_FRAC);
    //if (logical_bytes < deduped_bytes) {
    //    // we did not measure something properly, for now ignoring
    //    LOGTRACE << "logical bytes " << logical_bytes << " < deduped bytes "
    //            << deduped_bytes;
    //    return 0;
    //}
    //return (logical_bytes - deduped_bytes);
    /**
     * But this was problematic since the calculation depends
     * upon a stats from different services which may not always
     * be delivered on time. Better to capture the raw data and
     * let the stats consumers calculate what they want.
     */
    return slot.getTotal(STAT_DM_CUR_PBYTES);
}

fds_uint64_t StatHelper::getTotalDomainDedupBytesFrac(const StatSlot &slot) {
    return slot.getTotal(STAT_SM_CUR_DOMAIN_DEDUP_BYTES_FRAC);
}

fds_uint64_t StatHelper::getTotalDedupBytes(const StatSlot &slot) {
    return slot.getTotal(STAT_SM_CUR_DEDUP_BYTES);
}

// we do not include user-defined metadata
fds_uint64_t StatHelper::getTotalMetadataBytes(const StatSlot& slot) {
    fds_uint64_t tot_objs = slot.getTotal(STAT_DM_CUR_POBJECTS);
    fds_uint64_t tot_blobs = slot.getTotal(STAT_DM_CUR_BLOBS);
    // approx -- asume 20 bytes for blob name
    fds_uint64_t header_bytes = 20 + 8*3 + 4;
    return (tot_blobs * header_bytes + tot_objs * 24);
}

fds_uint64_t StatHelper::getTotalBlobs(const StatSlot& slot) {
    return slot.getTotal(STAT_DM_CUR_BLOBS);
}

fds_uint64_t StatHelper::getTotalLogicalObjects(const StatSlot &slot) {
    return slot.getTotal(STAT_DM_CUR_LOBJECTS);
}

fds_uint64_t StatHelper::getTotalPhysicalObjects(const StatSlot &slot) {
    return slot.getTotal(STAT_DM_CUR_POBJECTS);
}

double StatHelper::getAverageBytesInBlob(const StatSlot& slot) {
    double tot_bytes = slot.getTotal(STAT_DM_CUR_LBYTES);
    double tot_blobs = slot.getTotal(STAT_DM_CUR_BLOBS);
    if (tot_blobs == 0) return 0;
    return tot_bytes / tot_blobs;
}

double StatHelper::getAverageObjectsInBlob(const StatSlot& slot) {
    double tot_objects = slot.getTotal(STAT_DM_CUR_LOBJECTS);
    double tot_blobs = slot.getTotal(STAT_DM_CUR_BLOBS);
    if (tot_blobs == 0) return 0;
    return tot_objects / tot_blobs;
}

/**
 * We report gets from SSD as actual SSD gets and gets from cache
 */
fds_uint64_t StatHelper::getTotalSsdGets(const StatSlot& slot) {
    fds_uint64_t ssdGets = slot.getCount(STAT_SM_GET_SSD);
    fds_uint64_t amCacheGets = slot.getCount(STAT_AM_GET_CACHED_OBJ);
    return (ssdGets + amCacheGets);
}

fds_bool_t StatHelper::getQueueFull(const StatSlot& slot) {
    return (slot.getMin(STAT_AM_QUEUE_BACKLOG) > 4);  // random constant
}

fds_uint64_t StatHelper::getQueueBacklog(const StatSlot& slot) {
    return slot.getTotal(STAT_AM_QUEUE_BACKLOG);
}

fds_uint64_t StatHelper::getQueueWaitTime(const StatSlot &slot) {
    return slot.getTotal(STAT_AM_QUEUE_WAIT);
}

/**
 * Aggregate fine-grained stats into coarse-grained stats.
 *
 * By default, this runs according to FdsStatFGPeriodSec.
 */
void VolStatsTimerTask::runTimerTask() {
    aggr_->processStats();
}


/**
 * Summarize and stream stats according to registration.
 *
 * By default, this runs according to FdsStatPushAndAggregatePeriodSec.
 */
void StatStreamTimerTask::runTimerTask() {
    LOGTRACE << "Streaming stats for registration '" << reg_->id << "'";

    std::vector<fpi::volumeDataPoints> dataPoints;

    /**
     * If the stat stream registration object contains
     * a list of specified volumes, we operate on just
     * those. Otherwise, we operate on all volumes registered
     * with our aggregator.
     */
    std::vector<fds_volid_t> volumes;
    if (reg_->volumes.empty()) {
        for (auto i : statStreamAggr_.volstats_map_) {
            volumes.push_back(i.first);
        }
        cleanupVolumes(volumes);  // cleanup in case volumes were removed
    } else {
        for (auto i : reg_->volumes) {
            volumes.push_back(fds_volid_t(i));
        }
    }

    bool logLocal = reg_->method == std::string("log-local");
    for (auto volId : volumes) {
        //LOGTRACE << "Generating stream for volume <" << volId << ">.";
        std::vector<StatSlot> slots;
        std::map<fds_uint64_t, std::vector<fpi::DataPointPair> > volDataPointsMap;  // NOLINT
        VolumeStats::ptr volStat = statStreamAggr_.getVolumeStats(volId);
        if (!volStat) {
            GLOGTRACE << "Cannot get stat volume history for id '" << volId << "'";
            continue;
        }
        const std::string & volName = dataManager_.volumeName(volId);

        /**
         * For non-logLocal registrations, we'll always stream the fine-grained stats because they
         * are automatically configured as such. For logLocal registrations, we'll stream either
         * the fine-grained or coarse grained depending upon the stream period designated in the registration message.
         */
        VolumePerfHistory::ptr hist = nullptr;
        if (logLocal) {
            hist = (StatConstants::singleton()->FdsStatFGStreamPeriodFactorSec*1 == reg_->sample_freq_seconds) ?
                   volStat->finegrain_hist_ : volStat->coarsegrain_hist_;
        } else {
            hist = volStat->finegrain_hist_;
        }

        /**
         * Where did we leave off?
         */
        auto last_rel_sec = (vol_last_rel_sec_.count(volId) > 0) ? vol_last_rel_sec_[volId] :
                             std::numeric_limits<fds_uint64_t>::max(); // Indicates first time.

        /**
         * For reporting fine-grained stats, we want to ignore the last few seconds of stat generation
         * to give remote services enough time to send them to us for aggregation. Otherwise, they get
         * left out!
         *
         * How many seconds should we ignore? We'll wait some multiple of the number of seconds configured
         * for the period, the expiration of which causes remote services to push their stats to us and triggers
         * us to aggregate them for the volume.
         */
        auto last_secs_to_ignore = (hist == volStat->finegrain_hist_) ?
                                    StatConstants::singleton()->FdsStatCollectionWaitMult *
                                            StatConstants::singleton()->FdsStatPushAndAggregatePeriodSec :
                                    0;

        vol_last_rel_sec_[volId] = hist->toSlotList(slots, last_rel_sec, last_secs_to_ignore);

        for (auto slot : slots) {
            //LOGTRACE << "Processing slot:\n" << slot;
            fds_uint64_t timestamp = hist->getTimestamp((slot).getRelSeconds());

            timestamp /= NANOS_IN_SECOND;

            fpi::DataPointPair putsDP;
            putsDP.key = "Puts";
            putsDP.value = StatHelper::getCountObjPuts(slot);
            volDataPointsMap[timestamp].push_back(putsDP);

            fpi::DataPointPair moputsDP;
            moputsDP.key = "Explicit Tx Blob Puts";
            moputsDP.value = StatHelper::getCountExplicitTxBlobPuts(slot);
            volDataPointsMap[timestamp].push_back(moputsDP);

            fpi::DataPointPair soputsDP;
            soputsDP.key = "Implicit Tx Blob Puts";
            soputsDP.value = StatHelper::getCountImplicitTxBlobPuts(slot);
            volDataPointsMap[timestamp].push_back(soputsDP);

            fpi::DataPointPair getsDP;
            getsDP.key = "Gets";
            getsDP.value = StatHelper::getCountObjGets(slot);
            volDataPointsMap[timestamp].push_back(getsDP);

            fpi::DataPointPair cgetsDP;
            cgetsDP.key = "Cached Obj Gets";
            cgetsDP.value = StatHelper::getCountCachedObjGets(slot);
            volDataPointsMap[timestamp].push_back(cgetsDP);

            fpi::DataPointPair ssdgetsDP;
            ssdgetsDP.key = "SSD Obj Gets";
            ssdgetsDP.value = StatHelper::getCountSSDObjGets(slot);
            volDataPointsMap[timestamp].push_back(ssdgetsDP);

            fpi::DataPointPair blobgetsDP;
            blobgetsDP.key = "Blob Gets";
            blobgetsDP.value = StatHelper::getCountBlobGets(slot);
            volDataPointsMap[timestamp].push_back(blobgetsDP);

            fpi::DataPointPair qfullDP;
            qfullDP.key = "Queue Full";
            qfullDP.value = StatHelper::getQueueFull(slot);
            volDataPointsMap[timestamp].push_back(qfullDP);

            fpi::DataPointPair qbacklogDP;
            qbacklogDP.key = "Queue Backlog";
            qbacklogDP.value = StatHelper::getQueueBacklog(slot);
            volDataPointsMap[timestamp].push_back(qbacklogDP);

            fpi::DataPointPair qwaitDP;
            qwaitDP.key = "Queue Waittime";
            qwaitDP.value = StatHelper::getQueueWaitTime(slot);
            volDataPointsMap[timestamp].push_back(qwaitDP);

            fpi::DataPointPair ssdGetsDP;
            ssdGetsDP.key = "SSD Gets";
            ssdGetsDP.value = StatHelper::getTotalSsdGets(slot);
            volDataPointsMap[timestamp].push_back(ssdGetsDP);

            fpi::DataPointPair hddGetsDP;
            hddGetsDP.key = "HDD Gets";
            hddGetsDP.value = getsDP.value - ssdGetsDP.value;
            volDataPointsMap[timestamp].push_back(hddGetsDP);

            fpi::DataPointPair logicalBytesDP;
            logicalBytesDP.key = "Logical Bytes";
            logicalBytesDP.value = StatHelper::getTotalLogicalBytes(slot);
            volDataPointsMap[timestamp].push_back(logicalBytesDP);

            fpi::DataPointPair physicalBytesDP;
            physicalBytesDP.key = "Physical Bytes";
            physicalBytesDP.value = StatHelper::getTotalPhysicalBytes(slot);
            volDataPointsMap[timestamp].push_back(physicalBytesDP);

            fpi::DataPointPair domaindedupBytesDP;
            domaindedupBytesDP.key = "Domain Dedup Bytes Fraction";
            domaindedupBytesDP.value = StatHelper::getTotalDomainDedupBytesFrac(slot);
            volDataPointsMap[timestamp].push_back(domaindedupBytesDP);

            fpi::DataPointPair dedupBytesDP;
            dedupBytesDP.key = "Dedup Bytes";
            dedupBytesDP.value = StatHelper::getTotalDedupBytes(slot);
            volDataPointsMap[timestamp].push_back(dedupBytesDP);

            fpi::DataPointPair mdBytesDP;
            mdBytesDP.key = "Metadata Bytes";
            mdBytesDP.value = StatHelper::getTotalMetadataBytes(slot);
            volDataPointsMap[timestamp].push_back(mdBytesDP);

            fpi::DataPointPair blobsDP;
            blobsDP.key = "Blobs";
            blobsDP.value = StatHelper::getTotalBlobs(slot);
            volDataPointsMap[timestamp].push_back(blobsDP);

            fpi::DataPointPair logicalObjectsDP;
            logicalObjectsDP.key = "Objects";
            logicalObjectsDP.value = StatHelper::getTotalLogicalObjects(slot);
            volDataPointsMap[timestamp].push_back(logicalObjectsDP);

            fpi::DataPointPair physicalObjectsDP;
            physicalObjectsDP.key = "Physical Objects";
            physicalObjectsDP.value = StatHelper::getTotalPhysicalObjects(slot);
            volDataPointsMap[timestamp].push_back(physicalObjectsDP);

            fpi::DataPointPair aveBlobSizeDP;
            aveBlobSizeDP.key = "Ave Blob Size";
            aveBlobSizeDP.value = StatHelper::getAverageBytesInBlob(slot);
            volDataPointsMap[timestamp].push_back(aveBlobSizeDP);

            fpi::DataPointPair aveObjectsDP;
            aveObjectsDP.key = "Ave Objects per Blob";
            aveObjectsDP.value = StatHelper::getAverageObjectsInBlob(slot);
            volDataPointsMap[timestamp].push_back(aveObjectsDP);

            fpi::DataPointPair avDP;
            avDP.key = "Attach Vol";
            avDP.value = StatHelper::getCountAttachVol(slot);
            volDataPointsMap[timestamp].push_back(avDP);

            fpi::DataPointPair dvDP;
            dvDP.key = "Detach Vol";
            dvDP.value = StatHelper::getCountDetachVol(slot);
            volDataPointsMap[timestamp].push_back(dvDP);

            fpi::DataPointPair vstatDP;
            vstatDP.key = "Vol Stat";
            vstatDP.value = StatHelper::getCountVolStat(slot);
            volDataPointsMap[timestamp].push_back(vstatDP);

            fpi::DataPointPair pvmetaDP;
            pvmetaDP.key = "Put Vol MetaD";
            pvmetaDP.value = StatHelper::getCountPutVolMeta(slot);
            volDataPointsMap[timestamp].push_back(pvmetaDP);

            fpi::DataPointPair gvmetaDP;
            gvmetaDP.key = "Get Vol MetaD";
            gvmetaDP.value = StatHelper::getCountGetVolMeta(slot);
            volDataPointsMap[timestamp].push_back(gvmetaDP);

            fpi::DataPointPair gvcontDP;
            gvcontDP.key = "Get Vol Contents";
            gvcontDP.value = StatHelper::getCountGetVolContents(slot);
            volDataPointsMap[timestamp].push_back(gvcontDP);

            fpi::DataPointPair starttxDP;
            starttxDP.key = "Start Tx";
            starttxDP.value = StatHelper::getCountStartBlobTx(slot);
            volDataPointsMap[timestamp].push_back(starttxDP);

            fpi::DataPointPair committxDP;
            committxDP.key = "Commit Tx";
            committxDP.value = StatHelper::getCountCommitBlobTx(slot);
            volDataPointsMap[timestamp].push_back(committxDP);

            fpi::DataPointPair aborttxDP;
            aborttxDP.key = "Abort Tx";
            aborttxDP.value = StatHelper::getCountAbortBlobTx(slot);
            volDataPointsMap[timestamp].push_back(aborttxDP);

            fpi::DataPointPair dblobDP;
            dblobDP.key = "Delete Blob";
            dblobDP.value = StatHelper::getCountDeleteBlob(slot);
            volDataPointsMap[timestamp].push_back(dblobDP);

            fpi::DataPointPair rblobDP;
            rblobDP.key = "Rename Blob";
            rblobDP.value = StatHelper::getCountRenameBlob(slot);
            volDataPointsMap[timestamp].push_back(rblobDP);

            fpi::DataPointPair recentCapStdevDP;
            recentCapStdevDP.key = "Short Term Capacity Sigma";

            fpi::DataPointPair longCapStdevDP;
            longCapStdevDP.key = "Long Term Capacity Sigma";

            fpi::DataPointPair recentCapWmaDP;
            recentCapWmaDP.key = "Short Term Capacity WMA";

            fpi::DataPointPair recentPerfStdevDP;
            recentPerfStdevDP.key = "Short Term Perf Sigma";

            fpi::DataPointPair longPerfStdevDP;
            longPerfStdevDP.key = "Long Term Perf Sigma";

            fpi::DataPointPair recentPerfWma;
            recentPerfWma.key = "Short Term Perf WMA";

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
                /**
                 * We have local logging hard-coded to 1 minute and 1 hour. We recognize which
                 * we are working with by checking whether the sample frequency is set to the
                 * smallest allowed value.
                 */
                statStreamAggr_.writeStatsLog(volDataPoint, volId,
                        (StatConstants::singleton()->FdsStatFGStreamPeriodFactorSec*1 == reg_->sample_freq_seconds));
            }
            dataPoints.push_back(volDataPoint);
        }
    }

    if (dataPoints.size() == 0) {
        // nothing to send, no attached volumes
        return;
    }

    if (!logLocal) {
        fpi::SvcInfo info;
        if (!MODULEPROVIDER()->getSvcMgr()->getSvcInfo(reg_->dest, info)) {
            GLOGTRACE << "Failed to get svc info for uuid: '" << std::hex
                    << reg_->dest.svc_uuid << std::dec << "'";
        } else {
            // XXX: hard-coded to bind to java endpoint in AM
            EpInvokeRpc(fpi::StreamingClient, publishMetaStream, info.ip, 8911,
                    reg_->id, dataPoints);
        }
    }
}

StatStreamCancelTimerTask::StatStreamCancelTimerTask(FdsTimer& cancelTimer,
                                                     FdsTimer& streamTimer,
                                                     std::int32_t reg_id,
                                                     FdsTimerTaskPtr statStreamTask) :
                                FdsTimerTask(cancelTimer),
                                streamTimer(streamTimer),
                                reg_id(reg_id),
                                statStreamTask(statStreamTask) {
}

void StatStreamCancelTimerTask::runTimerTask() {
    LOGTRACE << "Streaming stats registration with id <" << reg_id << "> has expired.";

    streamTimer.cancel(statStreamTask);
}

}  // namespace fds

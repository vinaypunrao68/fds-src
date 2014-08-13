/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <fds_process.h>
#include <net/net-service.h>
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

    std::unordered_map<fds_uint64_t, std::vector<fpi::DataPointPair> > volDataPointsMap;
    const std::string & volName = dataMgr->volumeName(volid_);

    // note that we are passing time interval modules push stats to the aggregator
    // -- this is time interval back from current time that will be not included into
    // the list of slots returned, because these slots may not be fully filled yet, eg.
    // aggregator will receive stats from some modules but maybe not from all modules yet.
    last_print_ts_ = finegrain_hist_->toSlotList(slots, last_print_ts_, FdsStatPushPeriodSec);
    for (std::vector<StatSlot>::const_iterator cit = slots.cbegin();
         cit != slots.cend();
         ++cit) {
        fds_uint64_t timestamp = finegrain_hist_->getTimestamp((*cit).getTimestamp());

        timestamp /= 1000 * 1000;

        fpi::DataPointPair putsDP;
        putsDP.key = "Puts";
        putsDP.value = StatHelper::getTotalPuts(*cit);
        volDataPointsMap[timestamp].push_back(putsDP);

        fpi::DataPointPair getsDP;
        getsDP.key = "Gets";
        getsDP.value = StatHelper::getTotalGets(*cit);
        volDataPointsMap[timestamp].push_back(getsDP);

        fpi::DataPointPair logicalBytesDP;
        logicalBytesDP.key = "Logical Bytes";
        logicalBytesDP.value = StatHelper::getTotalLogicalBytes(*cit);
        volDataPointsMap[timestamp].push_back(logicalBytesDP);

        fpi::DataPointPair blobsDP;
        blobsDP.key = "Blobs";
        blobsDP.value = StatHelper::getTotalBlobs(*cit);
        volDataPointsMap[timestamp].push_back(blobsDP);

        fpi::DataPointPair objectsDP;
        objectsDP.key = "Objects";
        objectsDP.value = StatHelper::getTotalObjects(*cit);
        volDataPointsMap[timestamp].push_back(objectsDP);

        fpi::DataPointPair aveBlobSizeDP;
        aveBlobSizeDP.key = "Ave Blob Size";
        aveBlobSizeDP.value = StatHelper::getAverageBytesInBlob(*cit);
        volDataPointsMap[timestamp].push_back(aveBlobSizeDP);

        fpi::DataPointPair aveObjectsDP;
        aveObjectsDP.key = "Ave Objects per Blob";
        aveObjectsDP.value = StatHelper::getAverageObjectsInBlob(*cit);
        volDataPointsMap[timestamp].push_back(aveObjectsDP);

        // TODO(Anna) add slot to 1 hour history
        // debugging for now, remove when we start printing this to file
        LOGNOTIFY << "[" << finegrain_hist_->getTimestamp((*cit).getTimestamp()) << "], "
                  << "Volume " << std::hex << volid_ << std::dec << ", "
                  << "Puts " << StatHelper::getTotalPuts(*cit) << ", "
                  << "Gets " << StatHelper::getTotalGets(*cit) << ", "
                  << "Logical bytes " << StatHelper::getTotalLogicalBytes(*cit) << ", "
                  << "Blobs " << StatHelper::getTotalBlobs(*cit) << ", "
                  << "Objects " << StatHelper::getTotalObjects(*cit) << ", "
                  << "Ave blob size " << StatHelper::getAverageBytesInBlob(*cit) << " bytes, "
                  << "Ave objects in blobs " << StatHelper::getAverageObjectsInBlob(*cit);
    }

    std::vector<fpi::volumeDataPoints> dataPoints;
    for (auto dp : volDataPointsMap) {
        fpi::volumeDataPoints volDataPoint;
        volDataPoint.volume_name = volName;
        volDataPoint.timestamp = dp.first;
        volDataPoint.meta_list = dp.second;

        dataPoints.push_back(volDataPoint);
    }

    fpi::SvcUuid dest;
    fds_uint32_t regId;
    Error err = dataMgr->statStreamAggregator()->getStatStreamRegDetails(volid_, dest, regId);
    if (!err.ok()) {
        LOGERROR << "Failed to get registration details for volume '" << volName << "'";
        return;
    }

    EpInvokeRpc(fpi::StreamingClient, publishMetaStream, dest, 0, 2, regId, dataPoints);

    // log to 1-minute file
    // if enough time passed, also log to 1hour file
}



StatStreamAggregator::StatStreamAggregator(char const *const name)
        : Module(name),
          finestat_slotsec_(FdsStatPeriodSec),
          finestat_slots_(65) {
     // const fpi::volumeDataPointsPtr volStatData;
    // align timestamp to the length of finestat slot
    start_time_ = util::getTimeStampNanos();
    fds_uint64_t freq_nanos = finestat_slotsec_ * 1000000000;
    start_time_ = start_time_ / freq_nanos;
    start_time_ = start_time_ * freq_nanos;

    // writeStatsLog(volStatData);
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
    fds_verify(false);
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
    Error err(ERR_OK);
    LOGDEBUG << "Adding streaming registration with id " << registration->id;

    SCOPEDWRITE(lockStatStreamRegsMap);
    statStreamRegistrations_[registration->id] = registration;
    for (auto volId : registration->volumes) {
        attachVolume(volId);
    }
    return err;
}

Error StatStreamAggregator::deregisterStream(fds_uint32_t reg_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Removing streaming registration with id " << reg_id;

    SCOPEDWRITE(lockStatStreamRegsMap);
    StatStreamRegistrationMap_t::iterator iter = statStreamRegistrations_.find(reg_id);
    if (statStreamRegistrations_.end() != iter) {
        fds_verify(iter->second);
        for (auto volId : iter->second->volumes) {
            detachVolume(volId);
        }
        statStreamRegistrations_.erase(iter);
    }
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
        err = (volstats->finegrain_hist_)->mergeSlots(vstats, remote_start_ts);
        fds_verify(err.ok());  // if err, prob de-serialize issue
        LOGDEBUG << *(volstats->finegrain_hist_);
    }
    return err;
}

Error StatStreamAggregator::writeStatsLog(const fpi::volumeDataPointsPtr& volStatData) {
    Error err(ERR_OK);
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    const std::string fileName = root->dir_user_repo() + "stat_hour.log";
    FILE   *pFile;

    pFile  = fopen((const char *)fileName.c_str(), "a+");
    fprintf(pFile, " volume: %s,", volStatData->volume_name.c_str());
    fprintf(pFile, " time: %lx,", volStatData->timestamp);

    std::vector<DataPointPair>::iterator pos;

    for (pos = volStatData->meta_list.begin(); pos != volStatData->meta_list.end(); ++pos)
        fprintf(pFile, " %s: %f,", pos->key.c_str(), pos->value);

    fprintf(pFile, " \n");
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

void VolStatsTimerTask::runTimerTask() {
    volstats_->processStats();
}
}  // namespace fds

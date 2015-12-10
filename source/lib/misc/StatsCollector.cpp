/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <vector>
#include <fds_process.h>
#include <net/SvcRequestPool.h>
#include <net/SvcMgr.h>
#include <fdsp/dm_api_types.h>
#include <StatsCollector.h>

namespace fds {

StatsCollector glStatsCollector(FdsStatPushPeriodSec,
                                FdsStatPeriodSec, 1);

class CollectorTimerTask : public FdsTimerTask {
  public:
    typedef enum {
        TM_PUSH_STATS,
        TM_SAMPLE_STATS,
        TM_PRINT_QOS
    } collector_timer_t;

    StatsCollector* collector_;
    collector_timer_t timer_type_;

    CollectorTimerTask(FdsTimer &timer,
                       StatsCollector* collector,
                       collector_timer_t type)
            : FdsTimerTask(timer)
    {
        collector_ = collector;
        timer_type_ = type;
    }
    ~CollectorTimerTask() {}

    virtual void runTimerTask() override;
};

StatsCollector::StatsCollector(fds_uint32_t push_sec,
                               fds_uint32_t stat_sampling_freq,
                               fds_uint32_t qos_sampling_freq)
        : push_interval_(push_sec),
          slotsec_qos_(qos_sampling_freq),
          slots_qos_(7),
          slotsec_stat_(stat_sampling_freq),
          pushTimer(new FdsTimer()),
          pushTimerTask(new CollectorTimerTask(*pushTimer, this,
                                               CollectorTimerTask::TM_PUSH_STATS)),
          qosTimer(new FdsTimer()),
          qosTimerTask(new CollectorTimerTask(*qosTimer, this,
                                              CollectorTimerTask::TM_PRINT_QOS)),
          sampleTimer(new FdsTimer()),
          sampleTimerTask(new CollectorTimerTask(*sampleTimer, this,
                                                 CollectorTimerTask::TM_SAMPLE_STATS))
{
    // stats are disabled by default
    qos_enabled_ = ATOMIC_VAR_INIT(false);
    stream_enabled_ = ATOMIC_VAR_INIT(false);

    // start time must be aligned to highest sampling frequency
    start_time_ = util::getTimeStampNanos();
    fds_uint64_t stat_sampling_freq64 = stat_sampling_freq;
    fds_uint64_t freq_nanos = stat_sampling_freq64 * NANOS_IN_SECOND;
    start_time_ = start_time_ / freq_nanos;
    start_time_ = start_time_ * freq_nanos;

    /**
     * For collecting stats to be pushed to other services, we will collect
     * as many generations of stat slots as can be sampled in a push
     * period plus the number of push periods we are willing to wait for
     * a delinquent pusher to get their stats to us before we publish.
     */
    fds_verify(slotsec_stat_ > 0);
    slots_stat_ = push_interval_ / slotsec_stat_ + FdsStatWaitFactor;

    svcMgr_ = nullptr;
    record_stats_cb_ = NULL;
    stream_stats_cb_ = NULL;
}

void StatsCollector::openQosFile(const std::string& name) {
    fds_verify(!statfile_.is_open());
    std::string dirname;

    // we will add timestamp to the name of file
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
    std::string nowstr = to_simple_string(now);
    std::size_t i = nowstr.find_first_of(" ");
    std::size_t k = nowstr.find_first_of(".");
    std::string ts_str("");
    if (i != std::string::npos) {
        ts_str = nowstr.substr(0, i);
        if (k == std::string::npos) {
            ts_str.append("-");
            ts_str.append(nowstr.substr(i+1));
        }
    }

    if (g_fdsprocess) {
        const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
        dirname = root->dir_fds_var_stats() + "stats-data";
        root->fds_mkdir(dirname.c_str());
    } else {
        // for testing
        dirname = ".";
    }

    std::string fname = dirname + std::string("//") + name + std::string("-")
            + ts_str + std::string(".stat");
    statfile_.open(fname.c_str(), std::ios::out | std::ios::app);
}

StatsCollector::~StatsCollector() {
    pushTimer->destroy();
    qosTimer->destroy();
    sampleTimer->destroy();
    if (statfile_.is_open()){
        statfile_.close();
    }
    qos_hist_map_.clear();
    stat_hist_map_.clear();
}

StatsCollector* StatsCollector::singleton() {
    return &glStatsCollector;
}

void StatsCollector::setSvcMgr(SvcMgr* svcMgr) {
    svcMgr_ = svcMgr;
}

void StatsCollector::startStreaming(record_svc_stats_t record_stats_cb,
                                    stream_svc_stats_t stream_stats_cb) {
    fds_bool_t was_enabled = atomic_exchange(&stream_enabled_, true);
    if (!was_enabled) {
        LOGDEBUG << "Start pushing stats to DMs: Start time " << start_time_;
        stream_stats_cb_ = stream_stats_cb;
        // start periodic timer to push stats to primary DM
        fds_bool_t ret = pushTimer->scheduleRepeated(pushTimerTask,
                                                     std::chrono::seconds(push_interval_));
        if (!ret) {
            LOGERROR << "Failed to schedule timer to push stats for "
                     << " streaming metadata; will not include this module "
                     << " stats to the streaming metadata";
        }

        // if callback is provided, start timer to sample stats
        if (record_stats_cb) {
            record_stats_cb_ = record_stats_cb;
            last_sample_ts_ = util::getTimeStampNanos();
            fds_bool_t ret = sampleTimer->scheduleRepeated(sampleTimerTask,
                                                 std::chrono::seconds(slotsec_stat_));
            if (!ret) {
                LOGERROR << "Failed to schedule timer to sample stats; "
                         << " some stats may not be included for streaming metadata";
            }
        }
    }
}

void StatsCollector::stopStreaming() {
    fds_bool_t was_enabled = atomic_exchange(&stream_enabled_, false);
    if (was_enabled) {
        pushTimer->cancel(pushTimerTask);
        if (record_stats_cb_) {
            sampleTimer->cancel(sampleTimerTask);
            record_stats_cb_ = NULL;
            stream_stats_cb_ = NULL;
        }
    }
}

fds_bool_t StatsCollector::isStreaming() const {
    return std::atomic_load(&stream_enabled_);
}

void StatsCollector::enableQosStats(const std::string& name) {
    fds_bool_t was_enabled = atomic_exchange(&qos_enabled_, true);
    if (!was_enabled) {
        LOGDEBUG << "Enabled Qos stats output: Start time " << start_time_;
        openQosFile(name);

        // start periodic timer to push stats to primary DM
        fds_bool_t ret = qosTimer->scheduleRepeated(qosTimerTask,
                             std::chrono::seconds(slotsec_qos_ * (slots_qos_ - 2)));
        if (!ret) {
            LOGERROR << "Failed to schedule timer to print qos stats";
        }
    }
}

void StatsCollector::disableQosStats() {
    fds_bool_t was_enabled = atomic_exchange(&qos_enabled_, false);
    if (was_enabled) {
        qosTimer->cancel(qosTimerTask);
        // print stats we gathered but have not yet printed yet
        print();
        // close stats file
        if (statfile_.is_open()){
            statfile_.close();
        }
    }
}

fds_bool_t StatsCollector::isQosStatsEnabled() const {
    return std::atomic_load(&qos_enabled_);
}

void StatsCollector::recordEvent(fds_volid_t volume_id,
                                 fds_uint64_t timestamp,
                                 FdsStatType event_type,
                                 fds_uint64_t value) {
    switch (event_type) {
        case STAT_AM_PUT_OBJ:       // end-to-end put in AM in usec
        case STAT_AM_GET_OBJ:       // end-to-end get in AM in usec
        case STAT_AM_QUEUE_FULL:    // que size (recorded when que size > threshold)
            if (isQosStatsEnabled()) {
                getQosHistory(volume_id)->recordEvent(timestamp,
                                                      event_type, value);
            }
            if (isStreaming()) {
                getStatHistory(volume_id)->recordEvent(timestamp,
                                                       event_type, value);
            }
            break;

        case STAT_AM_QUEUE_WAIT:    // wait time in queue in usec
        case STAT_AM_GET_BMETA:
        case STAT_AM_PUT_BMETA:
            if (isQosStatsEnabled()) {
                getQosHistory(volume_id)->recordEvent(timestamp,
                                                      event_type, value);
            }
            break;
        default:
            if (isStreaming()) {
                getStatHistory(volume_id)->recordEvent(timestamp,
                                                       event_type, value);
            }
    };
}

VolumePerfHistory::ptr StatsCollector::getQosHistory(fds_volid_t volid)
{
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr>::iterator it;
    read_synchronized(qh_lock_) {
        it = qos_hist_map_.find(volid);
        if (qos_hist_map_.end() != it) {
            fds_assert(it->second);
            return it->second;
        }
    }

    // history not found, create one
    VolumePerfHistory::ptr new_hist(new VolumePerfHistory(volid,
                                                          start_time_,
                                                          slots_qos_,
                                                          slotsec_qos_));
    write_synchronized(qh_lock_) {
        qos_hist_map_[volid] = new_hist;
    }

    return new_hist;
}

VolumePerfHistory::ptr StatsCollector::getStatHistory(fds_volid_t volid)
{
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr>::iterator it;
    read_synchronized(sh_lock_) {
        it  = stat_hist_map_.find(volid);
        if (stat_hist_map_.end() != it) {
            fds_assert(it->second);
            return it->second;
        }
    }

    // history not found, create one
    VolumePerfHistory::ptr new_hist(new VolumePerfHistory(volid,
                                                          start_time_,
                                                          slots_stat_,
                                                          slotsec_stat_));
    write_synchronized(sh_lock_) {
        stat_hist_map_[volid] = new_hist;
    }

    return new_hist;
}


void StatsCollector::print()
{
    if (!isQosStatsEnabled()) return;

    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr> snap_map;
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr>::const_iterator cit;

    read_synchronized(qh_lock_) {
        for (cit = qos_hist_map_.cbegin(); cit != qos_hist_map_.cend(); ++cit) {
            VolumePerfHistory::ptr snap = (cit->second)->getSnapshot();
            snap_map[cit->first] = snap;
        }
    }

    // print
    boost::posix_time::ptime now_local = boost::posix_time::microsec_clock::local_time();
    for (cit = snap_map.cbegin(); cit != snap_map.cend(); ++cit) {
        if (last_ts_qmap_.count(cit->first) == 0) {
            last_ts_qmap_[cit->first] = 0;
        }
        fds_uint64_t last_rel_sec = last_ts_qmap_[cit->first];
        last_rel_sec = cit->second->print(statfile_, now_local, last_rel_sec);
        last_ts_qmap_[cit->first] = last_rel_sec;
    }
}

//
// will call callback provided by a service, so that service can
// record service-specific stats
//
void StatsCollector::sampleStats() {
    fds_uint64_t stat_slot_nanos = slotsec_stat_*NANOS_IN_SECOND;
    last_sample_ts_ += stat_slot_nanos;

    if (record_stats_cb_) {
        record_stats_cb_(last_sample_ts_);
    }
}

//
// send stats for each volume to DM primary for the volume
//
void StatsCollector::sendStatStream() {
    if (!isStreaming()) return;
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr> snap_map;
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr>::const_iterator cit;

    /**
     * Get a static snapshot of our stats history.
     */
    read_synchronized(sh_lock_) {
        for (cit = stat_hist_map_.cbegin(); cit != stat_hist_map_.cend(); ++cit) {
            snap_map[cit->first] = (cit->second)->getSnapshot();
        }
    }

    // if stream_stats_cb_ callback is provided, then this is a primary DM
    // and we will stream directly using the callback
    // TODO(Anna) the assumption here is that DM only collect stats that
    // needs to be collected on primary DM, so streaming is also to a local
    // module. If we need to collect any stats on secondary DMs as well, this
    // method needs to be extended to stream to both local and remote DM
    if (stream_stats_cb_) {
        for (cit = snap_map.cbegin(); cit != snap_map.cend(); ++cit) {
            if (last_rel_sec_smap_.count(cit->first) == 0) {
                last_rel_sec_smap_[cit->first] = 0;
            }
            fds_uint64_t last_rel_sec = last_rel_sec_smap_[cit->first];
            std::vector<StatSlot> slot_list;
            last_rel_sec = cit->second->toSlotList(slot_list, last_rel_sec);
            last_rel_sec_smap_[cit->first] = last_rel_sec;
            // send to local stats aggregator
            stream_stats_cb_(start_time_, cit->first, slot_list);
        }
        return;
    }

    // send to primary DMs
    std::unordered_map<NodeUuid, fpi::StatStreamMsgPtr, UuidHash> msg_map;
    std::unordered_map<NodeUuid, fpi::StatStreamMsgPtr, UuidHash>::const_iterator msg_cit;
    for (cit = snap_map.cbegin(); cit != snap_map.cend(); ++cit) {
        DmtColumnPtr nodes;

        /**
         * Get our DM UUID.
         */
        try {
            nodes = svcMgr_->getDMTNodesForVolume(cit->first);
            if (nodes == nullptr) {
                LOGERROR << "Invalid DMT for volume Id " << cit->first;
                return;
            }
        } catch (const std::exception& e) {
            LOGERROR << "Stats Collector encountered exception during sending stats "
                     << e.what();
            return;
        }
        NodeUuid dm_uuid = nodes->get(0);

        /**
         * If this is the first time we've seen this DM, build a
         * StatStream message structure for it.
         */
        if (msg_map.count(dm_uuid) == 0) {
            fpi::StatStreamMsgPtr msg(new fpi::StatStreamMsg());
            msg->start_timestamp = start_time_;
            msg_map[dm_uuid] = msg;
        }

        /**
         * If not already set, initialize the last relative seconds
         * for this volume.
         */
        if (last_rel_sec_smap_.count(cit->first) == 0) {
            last_rel_sec_smap_[cit->first] = 0;
        }

        /**
         * Copy our volume stats to an FDSP payload structure, obtaining
         * the last relative seconds of the latest slot in those stats.
         *
         * We keep track of those last relative seconds so we know where
         * pick up next time around.
         */
        fpi::VolStatList volstats;
        fds_uint64_t last_rel_sec = last_rel_sec_smap_[cit->first];
        last_rel_sec = cit->second->toFdspPayload(volstats, last_rel_sec);
        (msg_map[dm_uuid]->volstats).push_back(volstats);
        last_rel_sec_smap_[cit->first] = last_rel_sec;

        LOGTRACE << "Sending stats to DM " << std::hex
                 << dm_uuid.uuid_get_val() << std::dec
                 << " " << *(cit->second);
    }

    // send messages to primary DMs
    for (msg_cit = msg_map.cbegin(); msg_cit != msg_map.cend(); ++msg_cit) {
        auto asyncStreamReq = gSvcRequestPool->newEPSvcRequest(
            (msg_cit->first).toSvcUuid());
        asyncStreamReq->setPayload(FDSP_MSG_TYPEID(fpi::StatStreamMsg), msg_cit->second);
        asyncStreamReq->invoke();
    }
}

void CollectorTimerTask::runTimerTask()
{
    if (timer_type_ == TM_PUSH_STATS) {
        collector_->sendStatStream();
    } else if (timer_type_ == TM_PRINT_QOS) {
        collector_->print();
    } else {
        fds_verify(timer_type_ == TM_SAMPLE_STATS);
        collector_->sampleStats();
    }
}

}  // namespace fds

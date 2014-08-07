/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <net/SvcRequestPool.h>
#include <StatsCollector.h>

namespace fds {

StatsCollector::StatsCollector(fds_uint32_t push_sec,
                               fds_uint32_t stat_sampling_freq,
                               fds_uint32_t qos_sampling_freq)
        : push_interval_(push_sec),
          slotsec_qos_(qos_sampling_freq),
          slots_qos_(7),
          slotsec_stat_(stat_sampling_freq),
          pushTimer(new FdsTimer()),
          pushTimerTask(new StatTimerTask(*pushTimer, this, true)),
          qosTimer(new FdsTimer()),
          qosTimerTask(new StatTimerTask(*qosTimer, this, false))
{
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

    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    std::string dirname(root->dir_fds_var_stats());
    root->fds_mkdir(dirname.c_str());

    std::string fname = dirname + std::string("//") + ts_str + std::string(".stat");
    statfile_.open(fname.c_str(), std::ios::out | std::ios::app);

    // stats are disabled by default
    qos_enabled_ = ATOMIC_VAR_INIT(false);
    stream_enabled_ = ATOMIC_VAR_INIT(false);

    // start time must be aligned to highest sampling frequency
    start_time_ = util::getTimeStampNanos();
    fds_uint64_t freq_nanos = stat_sampling_freq * 1000000000;
    start_time_ = start_time_ / freq_nanos;
    start_time_ = start_time_ * freq_nanos;

    LOGDEBUG << "Start time " << start_time_;

    om_client_ = NULL;
}

StatsCollector::~StatsCollector() {
    pushTimer->destroy();
    qosTimer->destroy();
    if (statfile_.is_open()){
        statfile_.close();
    }
    qos_hist_map_.clear();
    stat_hist_map_.clear();
}

void StatsCollector::registerOmClient(OMgrClient* omclient) {
    om_client_ = omclient;
}

void StatsCollector::startStreaming() {
    fds_bool_t was_enabled = atomic_exchange(&stream_enabled_, true);
    if (!was_enabled) {
        // start periodic timer to push stats to primary DM
        fds_bool_t ret = pushTimer->scheduleRepeated(pushTimerTask,
                                                     std::chrono::seconds(push_interval_));
        if (!ret) {
            LOGERROR << "Failed to schedule timer to push stats for "
                     << " streaming metadata; will not include this module "
                     << " stats to the streaming metadata";
        }
    }
}

void StatsCollector::stopStreaming() {
    fds_bool_t was_enabled = atomic_exchange(&stream_enabled_, false);
    if (was_enabled) {
        pushTimer->cancel(pushTimerTask);
    }
}

fds_bool_t StatsCollector::isStreaming() const {
    return std::atomic_load(&stream_enabled_);
}

void StatsCollector::enableQosStats() {
    fds_bool_t was_enabled = atomic_exchange(&qos_enabled_, true);
    if (!was_enabled) {
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
    }
}

fds_bool_t StatsCollector::isQosStatsEnabled() const {
    return std::atomic_load(&qos_enabled_);
}

void StatsCollector::recordEvent(fds_volid_t volume_id,
                                 fds_uint64_t timestamp,
                                 PerfEventType event_type,
                                 fds_uint64_t value) {
    switch (event_type) {
        case AM_PUT_OBJ_REQ:  // end-to-end put in AM
        case AM_GET_OBJ_REQ:  // end-to-end get in AM
            if (isQosStatsEnabled()) {
                getQosHistory(volume_id)->recordEvent(timestamp,
                                                      event_type, value);
            }
            if (isStreaming()) {
                getStatHistory(volume_id)->recordEvent(timestamp,
                                                       event_type, value);
            }
            break;
        default:
            // ignore
            fds_panic("unexpected event");
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
    fds_uint64_t now = util::getTimeStampNanos();
    for (cit = snap_map.cbegin(); cit != snap_map.cend(); ++cit) {
        if (last_ts_qmap_.count(cit->first) == 0) {
            last_ts_qmap_[cit->first] = 0;
        }
        fds_uint64_t last_rel_sec = last_ts_qmap_[cit->first];
        last_rel_sec = cit->second->print(statfile_, now, last_rel_sec);
        last_ts_qmap_[cit->first] = last_rel_sec;
    }
}

//
// send stats for each volume to DM primary for the volume
//
void StatsCollector::sendStatStream() {
    if (!isStreaming()) return;
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr> snap_map;
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr>::const_iterator cit;

    read_synchronized(sh_lock_) {
        for (cit = stat_hist_map_.cbegin(); cit != stat_hist_map_.cend(); ++cit) {
            snap_map[cit->first] = (cit->second)->getSnapshot();
        }
    }

    // send to primary DMs
    std::unordered_map<NodeUuid, fpi::StatStreamMsgPtr, UuidHash> msg_map;
    std::unordered_map<NodeUuid, fpi::StatStreamMsgPtr, UuidHash>::const_iterator msg_cit;
    for (cit = snap_map.cbegin(); cit != snap_map.cend(); ++cit) {
        DmtColumnPtr nodes = om_client_->getDMTNodesForVolume(cit->first);
        NodeUuid dm_uuid = nodes->get(0);
        if (msg_map.count(dm_uuid) == 0) {
            fpi::StatStreamMsgPtr msg(new StatStreamMsg());
            msg->start_timestamp = start_time_;
            msg_map[dm_uuid] = msg;
        }
        if (last_ts_smap_.count(cit->first) == 0) {
            last_ts_smap_[cit->first] = 0;
        }

        fpi::VolStatList volstats;
        fds_uint64_t last_rel_sec = last_ts_smap_[cit->first];
        last_rel_sec = cit->second->toFdspPayload(volstats, last_rel_sec);
        (msg_map[dm_uuid]->volstats).push_back(volstats);
        last_ts_smap_[cit->first] = last_rel_sec;

        LOGTRACE << "Sending stats to DM " << std::hex
                 << (msg_cit->first).uuid_get_val() << std::dec
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

void StatTimerTask::runTimerTask()
{
    if (b_push) {
        collector_->sendStatStream();
    } else {
        collector_->print();
    }
}

}  // namespace fds

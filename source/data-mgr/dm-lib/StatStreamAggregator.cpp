/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <net/net-service.h>
#include <StatStreamAggregator.h>

namespace fds {

StatStreamAggregator::StatStreamAggregator(char const *const name)
        : Module(name),
          finestat_slotsec_(60),
          finestat_slots_(16) {
    // align timestamp to the length of finestat slot
    start_time_ = util::getTimeStampNanos();
    fds_uint64_t freq_nanos = finestat_slotsec_ * 1000000000;
    start_time_ = start_time_ / freq_nanos;
    start_time_ = start_time_ * freq_nanos;
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

    // we will create histories when we get any stats for the volume

    // TODO(xxx) open file where we are going to log stats
    // for this volume
    return err;
}

//
// returns fine-grained history for a given volume, if history does not exist
// creates one
//
VolumePerfHistory::ptr
StatStreamAggregator::getFineGrainHistory(fds_volid_t volid) {
    read_synchronized(fh_lock_) {
        hist_map_t::iterator it  = finestats_map_.find(volid);
        if (finestats_map_.end() != it) {
            fds_assert(it->second);
            return it->second;
        }
    }
    // history not found, create one
    VolumePerfHistory::ptr new_hist(new VolumePerfHistory(volid,
                                                          start_time_,
                                                          finestat_slots_,
                                                          finestat_slotsec_));
    write_synchronized(fh_lock_) {
        finestats_map_[volid] = new_hist;
    }
    return new_hist;
}

Error StatStreamAggregator::detachVolume(fds_volid_t volume_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Will stop monitoring stats for vol " << std::hex
             << volume_id << std::dec;
    return err;
}

Error StatStreamAggregator::registerStream(fds_uint32_t reg_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Will start streaming stats: reg id " << reg_id;
    return err;
}

Error StatStreamAggregator::deregisterStream(fds_uint32_t reg_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Will stop streaming stats: reg id " << reg_id;
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
        VolumePerfHistory::ptr hist = getFineGrainHistory(vstats.volume_id);
        err = hist->mergeSlots(vstats, remote_start_ts);
        fds_verify(err.ok());  // if err, prob de-serialize issue
        LOGTRACE << *hist;
    }
    return err;
}

}  // namespace fds

/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_STATSCOLLECTOR_H_
#define SOURCE_INCLUDE_STATSCOLLECTOR_H_

#include <PerfHistory.h>
#include <fds_timer.h>

#include "EclipseWorkarounds.h"

namespace fds {

struct SvcMgr;

/**
 * A callback to a service (DM/SM/AM) that will be called periodically
 * to add sampled stats to the stats collector
 * @param[in] timestamp is timestamp to pass to StatsCollector::recordEvent()
 */
typedef std::function<void (fds_uint64_t timestamp)> record_svc_stats_t;

/**
 * A callback that will stream stats to an aggregator which is local to
 * this module (instead of sending thrift message over network)
 */
typedef std::function<void (fds_uint64_t start_timestamp,
                            fds_volid_t volume_id,
                            const std::vector<StatSlot>& slots)> stream_svc_stats_t;

/**
 * Per-module collector of systems stats (that are always collected),
 * which is a combination of high-level performance stats and capacity
 * stats and possibly other dynamic systems stats.
 *
 * This class is a singleton and provides methods for systems stats
 * collection. It periodically pushes the collected stats to the
 * module that does aggregation of systems stats from all modules.
 * There maybe multiple destination modules (DM), and stats are sent
 * by volume, depending which DM has a primary responsibility for a volume
 */
class StatsCollector : public boost::noncopyable {
  public:
    /**
     * @param[in] push_sec is an interval in seconds. Every push_sec
     * seconds stats are pushed to the aggregator module.
     * @param[in] stat_sampling_freq is an interval in seconds. Each stat is
     * sampled for streaming every stat_sampling_freq seconds.
     * @param[in] qos_sampling_freq is an interval in seconds. Each QoS stat is
     * sampled for logging every qos_sampling_freq seconds.
     * TODO(Anna) this is the default for all volumes; we should allow
     * override of this interval for each volume if needed, so that we
     * can push stats for different volumes with different frequencies
     */
    StatsCollector(fds_uint32_t push_sec,
                   fds_uint32_t stat_sampling_freq = 60,
                   fds_uint32_t qos_sampling_freq = 1);
    ~StatsCollector();

    static StatsCollector* singleton();

    void setSvcMgr(SvcMgr *svcMgr);

    /**
     * Start and stop collecting systems stats and pushing them to
     * aggregator module
     * @param[in] record_stats_cb is a callback to a service (SM/DM/AM)that
     * will be called periodically to sample service specific system stats;
     * such as deduped bytes per volume in SM. If a service does not do any
     * service specific sampling, pass NULL.
     */
    void startStreaming(record_svc_stats_t record_stats_cb,
                        stream_svc_stats_t stream_stats_cb);
    void stopStreaming();
    fds_bool_t isStreaming() const;

    /**
     * Start and stop collecting and printing fine-grained qos stats
     */
    void enableQosStats(const std::string& name);
    void disableQosStats();
    fds_bool_t isQosStatsEnabled() const;

    /**
     * Record one event for volume 'volume_id'
     * @param[in] volume_id volume ID
     * @param[in] timestamp timestamp in nanoseconds
     * @param[in] event_type type of the event
     * @param[in] value is value of the event
     */
    void recordEvent(fds_volid_t volume_id,
                     fds_uint64_t timestamp,
                     FdsVolStatType event_type,
                     fds_uint64_t value);

    void print();
    /**
     * Called on timer periodically to push stat history to
     * primary DMs
     */
    void sendStatStream();
    /**
     * Called on timer periodically to sample stats
     */
    void sampleStats();

  private:  // methods
    VolumePerfHistory::ptr getQosHistory(fds_volid_t volid);
    VolumePerfHistory::ptr getStatHistory(fds_volid_t volid);
    void openQosFile(const std::string& name);

  private:
    alignas(64) std::atomic<bool> qos_enabled_;
    alignas(64) std::atomic<bool> stream_enabled_;

    fds_uint32_t push_interval_;  // push meta interval in seconds
    fds_uint32_t slotsec_qos_;   // length of slot in seconds for qos history
    fds_uint32_t slots_qos_;     // number of slots in qos history
    fds_uint32_t slotsec_stat_;  // length of slot in seconds for stat history
    fds_uint32_t slots_stat_;    // number of slots in stat history

    /**
     * Fine-grained short-time histories of few specific stats for
     * qos testing (used to be collected by PerfStats class)
     * Currently, each slot it 1 second length
     * Those are filled by recordEvent() and recordEventNow()
     */
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr> qos_hist_map_;
    fds_rwlock qh_lock_;  // lock protecting volume history map
    std::unordered_map<fds_volid_t, fds_uint64_t> last_ts_qmap_;

    /**
     * Coarse-grained history of all volumes for which we are collecting stats.
     * These are pushed via stats streaming API.
     * Currently, each slot (sample) is 1 minute length
     * Those are either filled directly by recordEvent() and recordEventNow()
     * or on timer sampling of stats from performance framework.
     */
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr> stat_hist_map_;
    fds_rwlock sh_lock_;  // lock protecting volume history map
    std::unordered_map<fds_volid_t, fds_uint64_t> last_rel_sec_smap_;

    fds_uint64_t start_time_;  // history timestamps are relative to this time

    /**
     * File we log fine-grained qos stats
     */
    std::ofstream statfile_;

    /**
     * timer to push volumes' stats to primary DMs
     */
    FdsTimerPtr pushTimer;
    FdsTimerTaskPtr pushTimerTask;
    stream_svc_stats_t stream_stats_cb_;  // if streaming to same module
    std::unordered_map<fds_volid_t, fds_uint64_t> last_rel_sec_sstream_;

    /**
     * Timer and callback for a service to do any
     * service-specific sampling of stats
     */
    record_svc_stats_t record_stats_cb_;
    FdsTimerPtr sampleTimer;
    FdsTimerTaskPtr sampleTimerTask;
    fds_uint64_t next_sample_ts_;

    /**
     * Timer to print qos stats
     */
    FdsTimerPtr qosTimer;
    FdsTimerTaskPtr qosTimerTask;

    /* Reference to service manager mostly for dmt */
     SvcMgr *svcMgr_;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_STATSCOLLECTOR_H_


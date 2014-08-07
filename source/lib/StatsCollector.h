/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_STATSCOLLECTOR_H_
#define SOURCE_INCLUDE_STATSCOLLECTOR_H_

#include <PerfHistory.h>
#include <lib/OMgrClient.h>

namespace fds {

/**
 * Per-module collector of systems stats (that are always collected),
 * which is a combination of high-level performance stats and capacity
 * stats and possibly other dynamic systems stats.
 *
 * This class is a singleton and provides methods for systems stats
 * collection. It periodically pushes the collected stats to the
 * module that does aggregation of systems stats from all modules.
 * There maybe multiple destination modules (DM), and stats are sent
 * by volume, depending which DM has a primary reponsibility for a volume
 */
class StatsCollector : public boost::noncopyable {
  public:
    /**
     * @param[in] push_sec is the interval in seconds, every push_sec
     * stats are pushed to the aggregator module
     * Stats are kept just a bit longer than push_sec interval
     * TODO(Anna) this is the default for all volumes; we should allow
     * override this interval for each volume if needed, so that we
     * can push stats for different volume with different frequency
     */
    StatsCollector(fds_uint32_t push_sec,
                   fds_uint32_t meta_sampling_freq = 60,
                   fds_uint32_t qos_sampling_freq = 1);
    ~StatsCollector();

    static StatsCollector* statsCollector();

    /**
     * This is temporary until service layer can provide us
     * with DMT; Using om client to provide DMT so we know where
     * to push per-volume stats
     */
    void registerOmClient(OMgrClient* omclient);

    /**
     * Start collecting systems stats for volumes that have stats collection
     * enabled
     */
    void startCollecting();
    /**
     * Enables systems stats collection for a given volume
     */
    void enable(fds_volid_t volume_id);

    /**
     * Stop collecting any stats
     */
    void stopCollection();
    /**
     * Explicitly disable system stats collection for a specific volume
     * We may want to disable stats collection for volumes in DM
     * where DM is not primary.
     */
    void disable(fds_volid_t volume_id);

    fds_bool_t isCollectingStats();
    fds_bool_t isEnabled(fds_volid_t volume_id);

    /**
     * Record one event for volume 'volume_id'
     * @param[in] volume_id volume ID
     * @param[in] timestamp timestamp in nanoseconds
     * @param[in] event_type type of the event
     * @param[in] value is value of the event
     */
    void recordEvent(fds_volid_t volume_id,
                     fds_uint64_t timestamp,
                     PerfEventType event_type,
                     fds_uint64_t value);

    /**
     * Record one event for volume 'volume_id' with timestamp now
     * @param[in] volume_id volume ID
     * @param[in] event_type type of the event
     * @param[in] value is value of the event
     */
    void recordEventNow(fds_volid_t volume_id,
                        PerfEventType event_type,
                        fds_uint64_t value);

  private:
    std::atomic<bool> enabled_;
    fds_uint32_t default_push_freq;  // push frequency in seconds

    /**
     * Fine-grained short-time histories of few specific stats for
     * qos testing (used to be collected by PerfStats class)
     * Currently, each slot it 1 second length
     * Those are filled by recordEvent() and recordEventNow()
     */
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr> qos_hist_map_;
    fds_rwlock qh_lock;  // lock protecting volume history map

    /**
     * Coarse-graned history of all volumes we are collecting stats
     * those are pushed for metadata streaming API.
     * Currently, each slot in 1 minute length
     * Those are either filled directly by recordEvent() and recordEventNow()
     * or on timer sampling of stats from performance framework.
     */
    std::unordered_map<fds_volid_t, VolumePerfHistory::ptr> meta_hist_map_;
    fds_rwlock mh_lock_;  // lock protecting volume history map

    fds_uint64_t start_time;  // history timestamps are relative to this time

    /**
     * timer to push volumes' stats to DMs
     */
    // TODO(Anna)

    /**
     * OMclient so we can get DMT
     * does not own, gets passed via registerOmCLient
     */
    OMgrClient* om_client_;
};


}  // namespace fds

#endif  // SOURCE_INCLUDE_STATSCOLLECTOR_H_


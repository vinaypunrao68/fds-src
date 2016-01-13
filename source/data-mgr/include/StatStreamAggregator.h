/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_STATSTREAMAGGREGATOR_H_
#define SOURCE_DATA_MGR_INCLUDE_STATSTREAMAGGREGATOR_H_

#include <string>
#include <vector>
#include <unordered_map>

#include <util/Log.h>
#include <fds_error.h>
#include <concurrency/RwLock.h>
#include <fds_module.h>
#include <PerfHistory.h>
#include <fdsp/fds_stream_types.h>

namespace fds {

class StatStreamAggregator;

/**
 * Invokes the stats stream process after the expiration of a
 * specified interval repeatedly.
 */
class StatStreamTimerTask : public FdsTimerTask {
  public:
    typedef boost::shared_ptr<StatStreamTimerTask> ptr;

    StatStreamTimerTask(FdsTimer &timer,
                        fpi::StatStreamRegistrationMsgPtr reg,
                        StatStreamAggregator & statStreamAggr,
                        DataMgr& dataManager);
    virtual ~StatStreamTimerTask() {}

    virtual void runTimerTask() override;

  private:
    void cleanupVolumes(const std::vector<fds_volid_t>& cur_vols);

  private:
    DataMgr& dataManager_;
    fpi::StatStreamRegistrationMsgPtr reg_;
    StatStreamAggregator & statStreamAggr_;
    std::unordered_map<fds_volid_t, fds_uint64_t> vol_last_rel_sec_;
};

/**
 * Cancels the StatStreamTimerTask after the expiration of
 * a given duration.
 */
class StatStreamCancelTimerTask : public FdsTimerTask {
    public:
        typedef boost::shared_ptr<StatStreamCancelTimerTask> ptr;

        StatStreamCancelTimerTask(FdsTimer& cancelTimer,
                                  FdsTimer& streamTimer,
                                  std::int32_t reg_id,
                                  FdsTimerTaskPtr statStreamTask);

        virtual ~StatStreamCancelTimerTask() {}

        virtual void runTimerTask() override;

    private:
        FdsTimer& streamTimer;
        std::int32_t reg_id;
        FdsTimerTaskPtr statStreamTask;
};

struct StatHistoryConfig {
    fds_uint32_t finestat_slotsec_;  // granularity of fine grain stats for all vols
    fds_uint32_t finestat_slots_;   // number of slots of fine grain stats for all vols
    fds_uint32_t coarsestat_slotsec_;  // granularity of coarse grain stats
    fds_uint32_t coarsestat_slots_;   // number of slots for coarse grain stats
    fds_uint32_t longstat_slotsec_;
    fds_uint32_t longstat_slots_;
};


/**
 * Per-volume stat histories, logging, etc.
 */
class VolumeStats {
  public:
    explicit VolumeStats(fds_volid_t volume_id,
                         fds_uint64_t start_time,
                         const StatHistoryConfig& config);

    ~VolumeStats();

    typedef boost::shared_ptr<VolumeStats> ptr;
    typedef boost::shared_ptr<const VolumeStats> const_ptr;

    /**
     * Fine-grain (cached) volume's history of stats
     * that are streamed out (we will log these)
     */
    VolumePerfHistory::ptr finegrain_hist_;

    /**
     * Per-hour volumes history of stats that are streamed
     * out (this is a cache of per-hour stats that we also log)
     */
    VolumePerfHistory::ptr coarsegrain_hist_;

    /**
     * Per-day volumes history of bytes added for firebreak
     */
    VolumePerfHistory::ptr longterm_hist_;

    /**
     * Queries currently cached stdev for performance and capacity growth.
     * This method may result in first updating the metrics if they are stale
     * and then returning them.
     * @param[out] recent_cap_stdev standard deviation of bytes added sampled
     * every coarse time interval (default = 1 hour) for the past N time intervals
     * (default = 24 hours)
     * @param[out] recent_cap_wma weighted moving average of bytes added sampled
     * every coarse time interval (default = 1 hour) for the past N time intervals
     * (default = 24 hours)
     * @param[out] long_cap_stdev standard deviation of bytes added / coarse time interval
     * sampled every long time interval (default = 1 day) for the past N time intervals
     * (default = 30 days)
     * @param[out] recent_perf_stdev standard deviation of number of obj gets + puts
     * sampled every coarse time interval (default = 1 hour) for the past N time
     * intervals (default = 24 hours)
     * @param[out] recent_perf_wma weighted moving average of number of obj gets + puts
     * sampled every coarse time interval (default = 1 hour) for the past N time
     * intervals (default = 24 hours)
     * @param[out] long_perf_stdev standard deviation of obj gets + puts / coarse time
     * interval sampled every long time interval (default = 1 day) for the past N time
     * intervals (default = 30 days)
     * 
     */
    void getFirebreakMetrics(double* recent_cap_stdev,
                             double* long_cap_stdev,
                             double* recent_cap_wma,
                             double* recent_perf_stdev,
                             double* long_perf_stdev,
                             double* recent_perf_wma);

    /**
     * Called periodically to process fine-grain stats:
     *  -- log to 1-min file
     *  -- add stats to coarse grain history
     *  -- log to 1-hour file if 1-hour worth of stats are ready
     */
    void processStats();

  private:  // methods
    void updateStdev(const std::vector<StatSlot>& slots,
                     double units_in_slot,
                     double* cap_stdev,
                     double* perf_stdev,
                     double* cap_wma,
                     double* perf_wma);

    void updateFirebreakMetrics();

  private:
    fds_volid_t volid_;  // volume id

    // cached stdev for capacity and performance
    double cap_recent_stdev_;
    double cap_long_stdev_;
    double cap_recent_wma_;
    double perf_recent_stdev_;
    double perf_long_stdev_;
    double perf_recent_wma_;

    /**
     * For processing finegrain stats
     */
    fds_uint64_t last_process_rel_secs_;
};

class StatHelper {
  public:
    static fds_uint64_t getTotalPuts(const StatSlot& slot);
    static fds_uint64_t getTotalGets(const StatSlot& slot);
    static fds_bool_t getQueueFull(const StatSlot& slot);
    static fds_uint64_t getTotalLogicalBytes(const StatSlot& slot);
    static fds_uint64_t getTotalBlobs(const StatSlot& slot);
    static fds_uint64_t getTotalObjects(const StatSlot& slot);
    static fds_uint64_t getTotalMetadataBytes(const StatSlot& slot);
    static double getAverageBytesInBlob(const StatSlot& slot);
    static double getAverageObjectsInBlob(const StatSlot& slot);
    static fds_uint64_t getTotalSsdGets(const StatSlot& slot);
    static fds_uint64_t getTotalPhysicalBytes(const StatSlot& slot);
};

/**
 * This is the module that keeps aggregated volumes' stats,
 * which could be pushed to AMs for streaming metadata API or queried
 * for any aggregated volume statistics.
 * The module accepts streamed stats histories from other modules and
 * aggregateds them; aggregation is depented on stat type (e.g. IOPS from
 * AMs are simply added; capacity related starts are often received from
 * primary DM/SM).
 *
 * We explicitly attach volume to aggregator, for which it is responsible
 * of aggregating stats and writing them to log, and accepting queries;
 * There is a separate registration for volume or set of volumes to start
 * pushing them to requester AM.
 */
class StatStreamAggregator : public HasModuleProvider, Module {
  public:
    typedef std::unordered_map<fds_uint32_t, fpi::StatStreamRegistrationMsgPtr>
            StatStreamRegistrationMap_t;

    StatStreamAggregator(CommonModuleProviderIf *modProvider,
                         char const* const name,
                         boost::shared_ptr<FdsConfig> fds_config,
                         DataMgr& dataManager);
    ~StatStreamAggregator();

    typedef boost::shared_ptr<StatStreamAggregator> ptr;
    typedef boost::shared_ptr<const StatStreamAggregator> const_ptr;
    typedef std::unordered_map<fds_volid_t, VolumeStats::ptr> volstats_map_t;

    /**
     * Module methods
     */
    virtual int mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    /**
     * Start aggregating and keeping track og stats
     * for the volume, including accepting stats stream for this volume
     * @param[in] volume_id volume ID
     */
    Error attachVolume(fds_volid_t volume_id);

    /**
     * Stop aggregating/keeping stats for the given volume
     * @param[in] volume_id volume ID
     */
    Error detachVolume(fds_volid_t volume_id);

    /**
     * write the stats  to the log file
     * @param[in] volumeDataPoints 
     */
    Error writeStatsLog(const fpi::volumeDataPoints& volStatData, fds_volid_t vol_id,
            bool isMin = true);
    Error volStatSync(NodeUuid dm_uuid, fds_volid_t vol_id);

    /**
     * Starts pushing of stats for a given set of volumes, with given
     * frequency
     * @param[in] Streaming registration
     */
    Error registerStream(fpi::StatStreamRegistrationMsgPtr registration);

    /**
     * Stop pushing stats for stat stream 'reg_id'
     * @param[in] reg_id registration id
     */
    Error deregisterStream(fds_uint32_t reg_id);

    /**
     * Handle stat stream from a single module
     * @param[in] stream_msg thrift data struct that contains a list
     * of histories for one of more volumes
     */
    Error handleModuleStatStream(const fpi::StatStreamMsgPtr& stream_msg);

    /**
     * Handle stat stream of a given volume from a local module
     * @param[in] start_timestamp is the timestamp of this volume's stream
     * all timestamps in slots vector are relative to start_timestamp
     * @param[in] volume_id volume id
     * @param[in] slots is a stat history of this volume
     */
    void handleModuleStatStream(fds_uint64_t start_timestamp,
                                fds_volid_t volume_id,
                                const std::vector<StatSlot>& slots);

    Error getStatStreamRegDetails(const fds_volid_t & volId, fpi::SvcUuid & amId,
            fds_uint32_t & regId);

    /**
     * called on timer to fill in coarse grain and long term stats
     */
    void processStats();

  private:  // methods
    VolumeStats::ptr getVolumeStats(fds_volid_t volid);

  private:  // members
    DataMgr& dataManager_;

    /**
     * Config for different histories we keep in terms of timescale
     */
    StatHistoryConfig hist_config;

    // timer to aggregate fine-grained slots into coarse-grained and long term slots
    FdsTimerPtr aggregate_stats_;
    FdsTimerTaskPtr aggregate_stats_task_;

    /**
     * Volume id to VolumeStats struct map; VolumeStats struct contains all
     * histores we cache for the volume and takes care of logging pre-volume
     * stats to a file
     */
    volstats_map_t volstats_map_;
    fds_rwlock volstats_lock_;   // lock protecting volstats map

    fds_uint64_t start_time_;  // timestamps in histories are relative to this time

    FdsTimer streamTimer_;  // For repeated stream task at each interval.
    FdsTimer cancelTimer_;  // To cancel the repeated stream task after duration.

    StatStreamRegistrationMap_t statStreamRegistrations_;
    std::unordered_map<fds_uint32_t, FdsTimerTaskPtr> statStreamTaskMap_;
    fds_rwlock lockStatStreamRegsMap;

    friend StatStreamTimerTask;
};
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_STATSTREAMAGGREGATOR_H_


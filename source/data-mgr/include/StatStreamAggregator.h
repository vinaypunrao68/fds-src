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


/**
 * Per-volume stat histories, logging, etc.
 */
class VolumeStats {
  public:
    explicit VolumeStats(fds_volid_t volume_id,
                         fds_uint64_t start_time,
                         fds_uint32_t finestat_slots,
                         fds_uint32_t finestat_slotsec);
    ~VolumeStats();

    typedef boost::shared_ptr<VolumeStats> ptr;
    typedef boost::shared_ptr<const VolumeStats> const_ptr;

    /**
     * Fine-grain (cached) volume's history of stats
     * that are streamed out (we will log these)
     * This will be set per-minute for now
     */
    VolumePerfHistory::ptr finegrain_hist_;

    /**
     * Per-hour volumes history of stats that are streamed
     * out (this is a cache of per-hour stats that we also log)
     * TODO(xxx) add stat history
     */

    /**
     * Per-day volumes history of bytes added for firebreak
     * TODO(xxx) add stat history
     */

    /**
     * TODO(xxx) add log file here, and timer to
     * log stat history for this volume
     */

    /**
     * Called periodically to process fine-grain stats:
     *  -- log to 1-min file
     *  -- add stats to coarse grain history
     *  -- log to 1-hour file if 1-hour worth of stats are ready
     */
    void processStats();

  private:
    fds_volid_t volid_;  // volume id
    /**
     * Timer to process fine-grain stats
     */
    FdsTimerPtr process_tm_;
    FdsTimerTaskPtr process_tm_task_;
    fds_uint32_t tmperiod_sec_;
    fds_uint64_t last_print_ts_;
};

class StatHelper {
  public:
    static fds_uint64_t getTotalPuts(const StatSlot& slot);
    static fds_uint64_t getTotalGets(const StatSlot& slot);
    static fds_uint64_t getTotalLogicalBytes(const StatSlot& slot);
    static fds_uint64_t getTotalBlobs(const StatSlot& slot);
    static fds_uint64_t getTotalObjects(const StatSlot& slot);
    static double getAverageBytesInBlob(const StatSlot& slot);
    static double getAverageObjectsInBlob(const StatSlot& slot);
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
class StatStreamAggregator : public Module {
  public:
    typedef std::unordered_map<fds_uint32_t, fpi::StatStreamRegistrationMsgPtr>
            StatStreamRegistrationMap_t;

    explicit StatStreamAggregator(char const *const name);
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
    Error writeStatsLog(const fpi::volumeDataPoints& volStatData);

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

  private:  // methods
    VolumeStats::ptr getVolumeStats(fds_volid_t volid);

  private:  // members
    /**
     * Config for fine-grain (cached) per-volume history of stats
     * This will be set per-minute for now
     */
    fds_uint32_t finestat_slotsec_;  // granularity of fine grain stats for all vols
    fds_uint32_t finestat_slots_;   // number of slots of fine grain stats for all vols
    /**
     * Config for coarser-grain (cached) per-volume history of stats
     * These will be set per-hour for now
     * TODO(xxx) add config
     */

    /**
     * Volume id to VolumeStats struct map; VolumeStats struct contains all
     * histores we cache for the volume and takes care of logging pre-volume
     * stats to a file
     */
    volstats_map_t volstats_map_;
    fds_rwlock volstats_lock_;   // lock protecting volstats map

    fds_uint64_t start_time_;  // timestamps in histories are relative to this time

    StatStreamRegistrationMap_t statStreamRegistrations_;
    fds_rwlock lockStatStreamRegsMap;
};
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_STATSTREAMAGGREGATOR_H_


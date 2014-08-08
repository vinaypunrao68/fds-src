/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_STATSTREAMAGGREGATOR_H_
#define SOURCE_DATA_MGR_INCLUDE_STATSTREAMAGGREGATOR_H_

#include <string>
#include <util/Log.h>
#include <fds_error.h>
#include <concurrency/RwLock.h>
#include <fds_module.h>
#include <PerfHistory.h>

namespace fds {


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
    explicit StatStreamAggregator(char const *const name);
    ~StatStreamAggregator();

    typedef boost::shared_ptr<StatStreamAggregator> ptr;
    typedef boost::shared_ptr<const StatStreamAggregator> const_ptr;
    typedef std::unordered_map<fds_volid_t, VolumePerfHistory::ptr> hist_map_t;

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
     * Starts pushing of stats for a given set of volumes, with given
     * frequency
     * TODO(xxx) add parameters, probably thrift data struct
     * @param[in] reg_id registration id
     */
    Error registerStream(fds_uint32_t reg_id);

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

  private:  // methods
    VolumePerfHistory::ptr getFineGrainHistory(fds_volid_t volid);

  private:  // members
    /**
     * Fine-grain (cached) per-volume history of stats
     * that are streamed out (we will log these)
     * This will be set per-minute for now
     */
    fds_uint32_t finestat_slotsec_;  // granularity of fine grain stats
    fds_uint32_t finestat_slots_;   // number of slots of fine grain stats we cache
    hist_map_t finestats_map_;
    fds_rwlock fh_lock_;   // lock protecting finestats_map_

    fds_uint64_t start_time_;  // timestamps in histories are relative to this time

    /**
     * Per-hour per-volume history of stats that are streamed
     * out (this is a cache of per-hour stats that we also log)
     */

    /**
     * Per-day per-volume history of bytes added for firebreak
     */
};
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_STATSTREAMAGGREGATOR_H_


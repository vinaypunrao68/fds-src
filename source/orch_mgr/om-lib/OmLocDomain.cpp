/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <orchMgr.h>
#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>
#include <OmResources.h>

namespace fds {

void
OM_NodeContainer::om_handle_perfstats_from_am(const fpi::FDSP_VolPerfHistListType &hist_list,
                                              const std::string start_timestamp)
{
    /*
     * Here is an assumption that a volume can only be attached to one AM,
     * need to revisit if this is not the case anymore
     * TODO(Anna) need to port to new stats class
     */
    /*
    for (uint i = 0; i < hist_list.size(); ++i) {
        fds_volid_t vol_uuid = hist_list[i].vol_uuid;
        for (uint j = 0; j < (hist_list[i].stat_list).size(); ++j) {
            FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                    << "OM: handle perfstat from AM for volume 0x"
                    << std::hex << vol_uuid << std::dec;

            am_stats->setStatFromFdsp(vol_uuid,
                                      start_timestamp,
                                      (hist_list[i].stat_list)[j]);
        }
    }
    */
}

// om_vol_get_stat
// ---------------
//
/* TODO(Anna) PerfStats class is replaced with new class, if
   we still want push fine-grain stats to OM, need to rewrite this method
static void
om_vol_get_stat(FDSP_BucketStatsRespTypePtr  resp,
                PerfStats                   *am_stats,
                fds_uint32_t                 perf_time_interval,
                boost::posix_time::ptime     ts,
                VolumeInfo::pointer          vol)
{
    FDSP_BucketStatType stat;
    om_vol_msg_t        vmsg;

    vmsg.vol_msg_code = FDSP_MSG_GET_BUCKET_STATS_RSP;
    vmsg.u.vol_stats  = &stat;
    vol->vol_fmt_message(&vmsg);

    stat.performance = am_stats->getAverageIOPS(
        vol->rs_get_uuid().uuid_get_val(), ts, perf_time_interval);

    (resp->bucket_stats_list).push_back(stat);

    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "sendBucketStats: will send stats for volume " << vol->vol_get_name()
            << ":0x" << std::hex << vol->rs_get_uuid().uuid_get_val() << std::dec
            << " perf " << stat.performance;
}
*/

/*
 * Get recent perf stats for all existing volumes and send them to the requesting node
 */
void
OM_NodeContainer::om_send_bucket_stats(fds_uint32_t perf_time_interval,
                                       const NodeUuid& node_uuid,
                                       fds_uint32_t req_cookie)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_AmAgent::pointer  am_node = local->om_am_agent(node_uuid);
    fds_panic("Need to implement wit new perf stats class");

    if (am_node == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::notification)
                << "Could not get AM node for " << std::hex
                << node_uuid.uuid_get_val() << std::dec;
        return;
    }
    FdspMsgHdrPtr               msg_hdr(new fpi::FDSP_MsgHdrType);
    fpi::FDSP_BucketStatsRespTypePtr buck_stats_rsp(new fpi::FDSP_BucketStatsRespType());

    am_node->init_msg_hdr(msg_hdr);
    msg_hdr->msg_code   = fpi::FDSP_MSG_GET_BUCKET_STATS_RSP;
    msg_hdr->dst_id     = fpi::FDSP_STOR_HVISOR;
    msg_hdr->req_cookie = req_cookie; /* copying from msg header that requested stats */
    msg_hdr->glob_volume_id = 0;      /* should ignore */

    boost::posix_time::ptime ts = boost::posix_time::second_clock::universal_time();
    /*
     * Here are some hardcored values assuming that AMs send stats every
     * (default slots num - 2)-second intervals, and OM receives
     * (default slots num - 1)-second interval worth of stats;
     * OM keeps 3*(default slots num) number of slots of stats. When we get
     * stats, we ignore last (default slot num) of slots in case OM did not
     * get the latest set of stats from all AMs, and calculate average performance
     * over a 'perf_time_interval' in seconds time interval.
     */

    /* Ignore most recent slots because OM may not receive latest stats from all AMs */
    /*
    ts -= boost::posix_time::seconds(am_stats->getSecondsInSlot() *
                                     FDS_STAT_DEFAULT_HIST_SLOTS);
    std::string temp_str = to_iso_extended_string(ts);
    std::size_t i = temp_str.find_first_of(",");
    std::string ts_str("");
    if (i != std::string::npos) {
        ts_str = temp_str.substr(0, i);
    } else {
        ts_str = temp_str;
    }
    */

    /*
     * The timestamp is the most recent timestamp of the interval we are getting the
     * average perf i.e., performance is average iops of interval
     * [timestamp - interval_length... timestamp]
     */
    /* TODO(Anna) Port to new perf stat class
    buck_stats_rsp->timestamp = ts_str;
    om_volumes->vol_foreach<
        FDSP_BucketStatsRespTypePtr,
        PerfStats *,
        fds_uint32_t,
        boost::posix_time::ptime
    >(buck_stats_rsp, am_stats, perf_time_interval, ts, om_vol_get_stat);


    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "Sending GetBucketStats response to node "
            << std::hex << node_uuid.uuid_get_val() << std::dec;

    am_node->getCpClient(&msg_hdr->session_uuid)->
        NotifyBucketStats(msg_hdr, buck_stats_rsp);
    */
}

}  // namespace fds

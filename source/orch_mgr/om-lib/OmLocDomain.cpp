/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <orchMgr.h>
#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>
#include <lib/PerfStats.h>
#include <OmResources.h>

namespace fds {

void
OM_NodeContainer::om_handle_perfstats_from_am(const FDSP_VolPerfHistListType &hist_list,
                                              const std::string start_timestamp)
{
    /*
     * Here is an assumption that a volume can only be attached to one AM,
     * need to revisit if this is not the case anymore
     */
    LOGNORMAL << " see perf stats from am ";
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
}

// om_vol_get_stat
// ---------------
//
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

    if (am_node == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::notification)
                << "Could not get AM node for " << std::hex
                << node_uuid.uuid_get_val() << std::dec;
        return;
    }
    FdspMsgHdrPtr               msg_hdr(new fpi::FDSP_MsgHdrType);
    FDSP_BucketStatsRespTypePtr buck_stats_rsp(new FDSP_BucketStatsRespType());

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

    /*
     * The timestamp is the most recent timestamp of the interval we are getting the
     * average perf i.e., performance is average iops of interval
     * [timestamp - interval_length... timestamp]
     */
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
}

// om_vol_fmt_stats
// ----------------
//
static void
om_vol_fmt_stats(std::ofstream            &file,
                 PerfStats                *am_stats,
                 int                      *vol_cnt,
                 boost::posix_time::ptime  ts,
                 VolumeInfo::pointer       vol)
{
    FDSP_BucketStatType stat;
    om_vol_msg_t        vmsg;
    fds_volid_t         vol_uuid;

    vmsg.vol_msg_code = FDSP_MSG_GET_BUCKET_STATS_RSP;
    vmsg.u.vol_stats  = &stat;
    vol->vol_fmt_message(&vmsg);

    vol_uuid = vol->rs_get_uuid().uuid_get_val();
    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
        << "printing stats for volume " << vol->vol_get_name();

    if (*vol_cnt> 0) {
        file << "," << std::endl;
    }

    /* TODO: the UI wants perf to be value between 0 - 100, so here we assume
     * performance no higher than 3200 IOPS; check again what's expected */
    int sla   = static_cast<int>(stat.sla / 32);
    int limit = static_cast<int>(stat.limit / 32);
    int perf  = static_cast<int>(am_stats->getAverageIOPS(vol_uuid, ts, 5) / 32);

    /* note we hardcoded IOPS average of 5 seconds,
     * we should probably have that as a parameter */
    file << "    {"
         << "\"id\": " << vol_uuid << ", "
         << "\"priority\": " << stat.rel_prio << ", "
         << "\"performance\": " << perf << ", "
         << "\"sla\": " << sla << ", "
         << "\"limit\": " << limit
         << "}";

    ++(*vol_cnt);
}

/* temp function to print recent perf stats of all existing volumes to json file */
void OM_NodeContainer::om_printStatsToJsonFile(void)
{
    int count = 0;
    boost::posix_time::ptime ts = boost::posix_time::second_clock::universal_time();

    /*
     * Here are some hardcored values assuming that AMs send stats every
     * (default slots num - 2)-second intervals, and OM receives
     * (default slots num - 1)-second interval worth of stats;
     * OM keeps 2*(default slots num) number of slots of stats. When we get
     * stats, we ignore last (default slot num) of slots in case OM did not
     * get the latest set of stats from all AMs, and calculate average performance
     * of 5 seconds that come before the last default number of slots worth of stats
     */

    /* ignore most recent slots because OM may not receive latest stats from all AMs */
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

    json_file << "{" << std::endl;
    json_file << "\"time\": \"" << ts_str << "\"" << std::endl;
    json_file << "\"volumes\":" << std::endl;
    json_file << "  [" << std::endl;

    /* for each volume, append perf info */
    om_volumes->vol_foreach<
        std::ofstream &,
        PerfStats *,
        int *,
        boost::posix_time::ptime
    >(json_file, am_stats, &count, ts, om_vol_fmt_stats);

    json_file << std::endl;
    json_file << "  ]" << std::endl;
    json_file << "}," << std::endl;
    json_file.flush();
}

}  // namespace fds

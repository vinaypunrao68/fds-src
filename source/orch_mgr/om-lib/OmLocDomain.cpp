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
    for (int i = 0; i < hist_list.size(); ++i) {
        double vol_uuid = hist_list[i].vol_uuid;
        for (int j = 0; j < (hist_list[i].stat_list).size(); ++j) {
            FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                    << "OM: handle perfstat from AM for volume "
                    << hist_list[i].vol_uuid;

            am_stats->setStatFromFdsp((fds_uint32_t)hist_list[i].vol_uuid,
                                      start_timestamp,
                                      (hist_list[i].stat_list)[j]);
        }
    }
}

/*
 * Get recent perf stats for all existing volumes and send them to the requesting node
 */
void
OM_NodeContainer::om_send_bucket_stats(fds_uint32_t perf_time_interval,
                                       std::string  dest_node_name,
                                       fds_uint32_t req_cookie)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    NodeUuid             node_uuid(fds_get_uuid64(dest_node_name));
    OM_AmAgent::pointer  am_node = local->om_am_agent(node_uuid);

    if (am_node == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "Could not get AM node for " << dest_node_name << std::endl;
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
#if 0
    /* for each volume, append perf info */
    for (auto it = volumeMap.begin(); it != volumeMap.end(); ++it) {
        VolumeInfo *pVolInfo = it->second;
        if (!pVolInfo && !pVolInfo->properties)
            continue;

        FDSP_BucketStatType stat;
        stat.vol_name = pVolInfo->vol_name;
        stat.sla = pVolInfo->properties->iops_min;
        stat.limit = pVolInfo->properties->iops_max;
        stat.performance = am_stats->getAverageIOPS(pVolInfo->volUUID,
                                                    ts,
                                                    perf_time_interval);
        stat.rel_prio = pVolInfo->properties->relativePrio;
        (buck_stats_rsp->bucket_stats_list).push_back(stat);

        FDS_PLOG_SEV(parent_log, fds_log::notification)
                << "sendBucketStats: will send stats for volume "
                << pVolInfo->vol_name
                << " perf " << stat.performance;
    }
#endif
    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "Sending GetBucketStats response to node "
            << dest_node_name << std::endl;

    am_node->getCpClient(&msg_hdr->session_uuid)->
        NotifyBucketStats(msg_hdr, buck_stats_rsp);
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
#if 0
    for (auto it = volumeMap.begin(); it != volumeMap.end(); ++it) {
        VolumeInfo *pVolInfo = it->second;
        if (!pVolInfo && !pVolInfo->properties)
            continue;

        FDS_PLOG(parent_log) << "printing stats for volume " << pVolInfo->vol_name;
        if (count > 0) {
            json_file << "," << std::endl;
        }

        /* TODO: the UI wants perf to be value between 0 - 100, so here we assume
         * performance no higher than 3200 IOPS; check again what's expected */
        int sla = static_cast<int>(pVolInfo->properties->iops_min / 32);
        int limit = static_cast<int>(pVolInfo->properties->iops_max / 32);
        int perf = static_cast<int>(
            am_stats->getAverageIOPS(pVolInfo->volUUID, ts, 5) / 32);

        /* note we hardcoded IOPS average of 5 seconds,
         * we should probably have that as a parameter */
        json_file << "    {"
                  << "\"id\": " << pVolInfo->volUUID << ", "
                  << "\"priority\": " << pVolInfo->properties->relativePrio << ", "
                  << "\"performance\": " << perf << ", "
                  << "\"sla\": " << sla << ", "
                  << "\"limit\": " << limit
                  << "}";

        ++count;
    }
#endif
    json_file << std::endl;
    json_file << "  ]" << std::endl;
    json_file << "}," << std::endl;
    json_file.flush();
}

}  // namespace fds

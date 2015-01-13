/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef __FDSP_H__
#define __FDSP_H__

namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace * FDS_ProtocolInterface

enum tier_prefetch_type_e
{
    PREFETCH_MRU,    /* most recently used to complement LRU */
    PREFETCH_RAND,   /* random eviction policy. */
    PREFETCH_ARC     /* arc algorithm */
}

enum tier_media_type_e
{
    TIER_MEIDA_NO_VAL = 0,
    TIER_MEDIA_DRAM,
    TIER_MEDIA_NVRAM,
    TIER_MEDIA_SSD,
    TIER_MEDIA_HDD,
    TIER_MEDIA_HYBRID,
    TIER_MEDIA_HYBRID_PREFCAP
}

struct tier_time_spec
{
    1: i32                      ts_sec;
    2: i32                      ts_min;
    /*
     * Hour encoding:
     *           0 : hourly.
     * 0x0000.001f : 1-24 regular hour of day.
     * 0x0000.0370 : 1-24 hour for range.
     * 0x0001.0000 : hour of day range cycle (e.g. 9-17h).
     * 0x0002.0000 : hour of day range interval.
     */
    3: i32                      ts_hour;
    /*
     * Day of month encoding:
     *           0 : daily.
     * 0x0000.001f : 1-31 regular day of month.
     * 0x0000.0370 : 1-31 day for range.
     * 0x0001.0000 : day of month range cycle.
     * 0x0002.0000 : day of month range interval.
     */
    4: i32                      ts_mday;
    /*
     * Day of week encoding:
     *           0 : daily.
     * 0x0000.0007 : 1-7 regular day of week.
     * 0x0000.0070 : 1-7 day for range.
     * 0x0001.0000 : day of week range cycle (e.g. M-F).
     * 0x0002.0000 : day of week range interval.
     */
    5: i32                      ts_wday;   /* day of the week. */
    /*
     * Month encoding:
     *           0 : monthly.
     * 0x0000.000f : 1-12 regular month.
     * 0x0000.00f0 : 1-12 month for range.
     * 0x0001.0000 : month range cycle (e.g. quarterly).
     * 0x0002.0000 : month range interval.
     */
    6: i32                      ts_mon;    /* month. */
    7: i32                      ts_year;   /* year. */
    8: i32                      ts_isdst;  /* daylight saving time. */
}

struct tier_pol_time_unit
{
    1: double                   tier_vol_uuid;
    2: bool                     tier_domain_policy;
    3: double                   tier_domain_uuid;
    4: tier_media_type_e        tier_media;
    5: tier_prefetch_type_e     tier_prefetch;
    6: i64                      tier_media_pct;
    7: tier_time_spec           tier_period;
}

struct tier_pol_audit
{
    1: i64                     tier_sla_min_iops;
    2: i64                     tier_sla_max_iops;
    3: i64                     tier_sla_min_latency;
    4: i64                     tier_sla_max_latency;
    5: i64                     tier_stat_min_iops;
    6: i64                     tier_stat_max_iops;
    7: i64                     tier_stat_avg_iops;
    8: i64                     tier_stat_min_latency;
    9: i64                     tier_stat_max_latency;
    10: i64                    tier_stat_avg_latency;

    11: i64                    tier_pct_ssd_iop;
    12: i64                    tier_pct_hdd_iop;
    13: i64                    tier_pct_ssd_capacity;
}

service orch_PolicyReq {
    void applyTierPolicy(1: tier_pol_time_unit policy);
    void auditTierPolicy(1: tier_pol_audit audit);
}

#endif

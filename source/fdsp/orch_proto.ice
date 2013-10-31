#ifndef __FDSP_H__
#define __FDSP_H__
#include <Ice/Identity.ice>
#include <Ice/BuiltinSequences.ice>

#pragma once
module orch_ProtocolInterface {

enum tier_prefetch_type_e
{
    PREFETCH_MRU,    /* most recently used to complement LRU */
    PREFETCH_RAND,   /* random eviction policy. */
    PREFETCH_ARC     /* arc algorithm */
};

enum tier_media_type_e
{
    TIER_MEIDA_NO_VAL = 0,
    TIER_MEDIA_DRAM,
    TIER_MEDIA_NVRAM,
    TIER_MEDIA_SSD,
    TIER_MEDIA_HDD,
    TIER_MEDIA_HYBRID
};

struct tier_time_spec
{
    int                      ts_sec;
    int                      ts_min;
    /*
     * Hour encoding:
     *           0 : hourly.
     * 0x0000.001f : 1-24 regular hour of day.
     * 0x0000.0370 : 1-24 hour for range.
     * 0x0001.0000 : hour of day range cycle (e.g. 9-17h).
     * 0x0002.0000 : hour of day range interval.
     */
    int                      ts_hour;
    /*
     * Day of month encoding:
     *           0 : daily.
     * 0x0000.001f : 1-31 regular day of month.
     * 0x0000.0370 : 1-31 day for range.
     * 0x0001.0000 : day of month range cycle.
     * 0x0002.0000 : day of month range interval.
     */
    int                      ts_mday;
    /*
     * Day of week encoding:
     *           0 : daily.
     * 0x0000.0007 : 1-7 regular day of week.
     * 0x0000.0070 : 1-7 day for range.
     * 0x0001.0000 : day of week range cycle (e.g. M-F).
     * 0x0002.0000 : day of week range interval.
     */
    int                      ts_wday;   /* day of the week. */
    /*
     * Month encoding:
     *           0 : monthly.
     * 0x0000.000f : 1-12 regular month.
     * 0x0000.00f0 : 1-12 month for range.
     * 0x0001.0000 : month range cycle (e.g. quarterly).
     * 0x0002.0000 : month range interval.
     */
    int                      ts_mon;    /* month. */
    int                      ts_year;   /* year. */
    int                      ts_isdst;  /* daylight saving time. */
};

struct tier_pol_time_unit
{
    double                   tier_vol_uuid;
    bool                     tier_domain_policy;
    double                   tier_domain_uuid;
    tier_media_type_e        tier_media;
    tier_prefetch_type_e     tier_prefetch;
    long                     tier_media_pct;
    tier_time_spec           tier_period;
};

struct tier_pol_audit
{
    long                     tier_sla_min_iops;
    long                     tier_sla_max_iops;
    long                     tier_sla_min_latency;
    long                     tier_sla_max_latency;
    long                     tier_stat_min_iops;
    long                     tier_stat_max_iops;
    long                     tier_stat_avg_iops;
    long                     tier_stat_min_latency;
    long                     tier_stat_max_latency;
    long                     tier_stat_avg_latency;

    long                     tier_pct_ssd_iop;
    long                     tier_pct_hdd_iop;
    long                     tier_pct_ssd_capacity;
};

interface orch_PolicyReq {
    void applyTierPolicy(tier_pol_time_unit policy);
    void auditTierPolicy(tier_pol_audit audit);
};

interface orch_PolicyResp {
    void applyTierPolicyResp(tier_pol_time_unit resp);
    void audiTierPolicyResp(tier_pol_audit audit);
};

};
#endif



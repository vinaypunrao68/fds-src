/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
/*
 * Systems stats types
 */
#ifndef SOURCE_INCLUDE_STATTYPES_H_
#define SOURCE_INCLUDE_STATTYPES_H_

#include <fdsp/common_constants.h>
#include "fds_typedefs.h" //NOLINT

namespace fds {

class StatConstants {
    public:
        StatConstants();
        static const StatConstants* singleton();

        /**
         * See the constructor for details about how and why these are set.
         */
        std::int32_t FdsStatLTPeriodSec;
        std::int32_t FdsStatLTSlotCnt;
        std::int32_t FdsStatFGPeriodSec;
        std::int32_t FdsStatFGPeriodMultToCG;
        std::int32_t FdsStatFGSlotCnt;
        std::int32_t FdsStatCGPeriodSec;
        std::int32_t FdsStatCGPeriodMultToLT;
        std::int32_t FdsStatCGSlotCnt;
        std::int32_t FdsStatPushAndAggregatePeriodSec;
        std::int32_t FdsStatCollectionWaitMult;
        std::int32_t FdsStatFGStreamPeriodFactorSec;
        std::int32_t FdsStatRunForever;
};

extern const StatConstants* g_stat_constants;

/**
 * These are Volume statistics. They only make sense when viewed within the context of the
 * Volume for which they were collected. other product entities for which we might wish to
 * collect stats include Nodes, Services, and Tenants.
 *
 * Since these IDs are what's recorded with the stats (as opposed to, say, the enum name) when
 * they are written out, probably it is best to always append to this list rather than insert
 * in the at the beginning or in the middle somewhere. Also, if a stat becomes deprecated,
 * it's probably best to leave in the enum (maybe renaming it somehow to indicate deprecation)
 * but not remove it.
 *
 * In short, once established for a stat, its probably best if the enum value for that stat
 * never changes. For if it does, it impacts all consumers of stats, not only the ones built
 * by FDS, but also those that may be built by customers or third party ISVs.
 */
typedef enum {
    STAT_AM_GET_OBJ,            // 0 - Data Object Get count including cached Data Objects
    STAT_AM_GET_CACHED_OBJ,     // 1 - Data Object Get from AM cache count
    STAT_AM_PUT_OBJ,            // 2 - Data Object count from FDS_PUT_BLOB and FDS_PUT_BLOB_ONCE I/O type
    STAT_AM_GET_BMETA,          // 3 - FDS_STAT_BLOB
    STAT_AM_PUT_BMETA,          // 4 - FDS_SET_BLOB_METADATA
    STAT_AM_QUEUE_BACKLOG,      // 5 - AM QoS Backlog
    STAT_AM_QUEUE_WAIT,         // 6 - Delay from AM receipt to dispatch.

    STAT_SM_GET_SSD,            // 7
    STAT_SM_CUR_DOMAIN_DEDUP_BYTES_FRAC,    // 8 - ObjSize * (domain_refcnt - 1) * vol_refcnt / domain_refcnt

    STAT_DM_BYTES_ADDED,        // 9
    STAT_DM_BYTES_REMOVED,      // 10
    STAT_DM_CUR_LBYTES,         // 11 - Not adjusted for dedup bytes.
    STAT_DM_CUR_LOBJECTS,       // 12 - Not adjusted for dedup Data Objects.
    STAT_DM_CUR_BLOBS,          // 13
    STAT_DM_CUR_PBYTES,         // 14 - (Data Object size) * (unique Data Objects referenced by the Volume)
    STAT_DM_CUR_POBJECTS,       // 15 - Total unique Data Objects referenced by the Volume.

    STAT_AM_ATTACH_VOL,         // 16 - FDS_ATTACH_VOL
    STAT_AM_DETACH_VOL,         // 17 - FDS_DETACH_VOL
    STAT_AM_VSTAT,              // 18 - FDS_STAT_VOLUME
    STAT_AM_PUT_VMETA,          // 19 - FDS_SET_VOLUME_METADATA
    STAT_AM_GET_VMETA,          // 20 - FDS_GET_VOLUME_METADATA
    STAT_AM_GET_VCONTENTS,      // 21 - FDS_VOLUME_CONTENTS
    STAT_AM_START_BLOB_TX,      // 22 - FDS_START_BLOB_TX
    STAT_AM_COMMIT_BLOB_TX,     // 23 - FDS_COMMIT_BLOB_TX
    STAT_AM_ABORT_BLOB_TX,      // 24 - FDS_ABORT_BLOB_TX
    STAT_AM_DELETE_BLOB,        // 25 - FDS_DELETE_BLOB
    STAT_AM_RENAME_BLOB,        // 26 - FDS_RENAME_BLOB
    STAT_AM_PUT_BLOB,           // 27 - FDS_PUT_BLOB
    STAT_AM_GET_BLOB,           // 28 - FDS_GET_BLOB
    STAT_AM_PUT_BLOB_ONCE,      // 29 - FDS_PUT_BLOB_ONCE

    STAT_SM_CUR_DEDUP_BYTES,    // 30 - (Data Object size) * (Volume ref count - 1)

    STAT_MAX_TYPE  // last entry in the enum
} FdsVolStatType;

class FdsStatHash {
  public:
    fds_uint32_t operator()(FdsVolStatType evt) const {
        return static_cast<fds_uint32_t>(evt);
    }
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_STATTYPES_H_

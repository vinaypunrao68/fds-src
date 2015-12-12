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
        std::int32_t FdsStatRunForever;
};

extern const StatConstants* g_stat_constants;

typedef enum {
    STAT_AM_GET_OBJ,
    STAT_AM_GET_CACHED_OBJ,
    STAT_AM_PUT_OBJ,
    STAT_AM_GET_BMETA,
    STAT_AM_PUT_BMETA,
    STAT_AM_QUEUE_FULL,
    STAT_AM_QUEUE_WAIT,

    STAT_SM_GET_SSD,
    STAT_SM_CUR_DEDUP_BYTES,

    STAT_DM_BYTES_ADDED,
    STAT_DM_BYTES_REMOVED,
    STAT_DM_CUR_TOTAL_BYTES,
    STAT_DM_CUR_TOTAL_OBJECTS,
    STAT_DM_CUR_TOTAL_BLOBS,

    STAT_MAX_TYPE  // last entry in the enum
} FdsStatType;

class FdsStatHash {
  public:
    fds_uint32_t operator()(FdsStatType evt) const {
        return static_cast<fds_uint32_t>(evt);
    }
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_STATTYPES_H_

/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
/*
 * Systems stats types
 */
#ifndef SOURCE_INCLUDE_STATTYPES_H_
#define SOURCE_INCLUDE_STATTYPES_H_

#include <fdsp/common_constants.h>
#include "fds_typedefs.h" //NOLINT

namespace fds {

/**
 * Defaults (later could be configurable) for
 * sampling and pushing stats from individual modules to
 * an aggregator.
 *
 * We establish a period for collecting "fine-grained" or FG stats and a
 * multiplier of that period used to define "coarse-grained" or CG stats.
 * ("Long-term" or LT stats are collected over a 24-hour period.)
 * Then we determine the history or number of generations of stats or "slots"
 * to keep in each case as a couple more than necessary to roll up into the
 * next more "coarse-grained" collection.
 */
const std::int32_t FdsStatLTPeriodSec{24*60*60};  // The period over which "Long-Term" stats are collected - every 24 hours.
const std::int32_t FdsStatLTSlotCnt{31};  // 30 days + 1

/**
 * FdsStatFGPeriodSec needs to be set to the value given for
 * FDS_ProtocolInterface::g_common_constants.STAT_STREAM_FINE_GRAINED_FREQUENCY_SECONDS.
 * Because of the way Thrift generates this defintiion, it cannot be used directly here
 * for this initialization.
 */
const std::int32_t FdsStatFGPeriodSec{60};  // The period over which "Fine-Grained" stats are collected.
const std::int32_t FdsStatFGPeriodMultToCG{10};  // The multiple of FG periods that define a CG period.
const std::int32_t FdsStatFGSlotCnt{FdsStatFGPeriodMultToCG + 2};  // Number of "slots" or generations of FG stats we keep in history. We'll keep a few more than necessary to roll into a CG slot.

const std::int32_t FdsStatCGPeriodSec{FdsStatFGPeriodMultToCG * FdsStatFGPeriodSec};  // The period over which "Coarse-Grained" stats are collected
const std::int32_t FdsStatCGPeriodMultToLT{FdsStatLTPeriodSec / FdsStatCGPeriodSec};  // The multiple of CG periods that define a LT period.
const std::int32_t FdsStatCGSlotCnt{FdsStatCGPeriodMultToLT + 2};  // Number of "slots" or generations of CG stats we keep in history. We'll keep a few more than necessary to roll into a LT slot.

const std::int32_t FdsStatPushAndAggregatePeriodSec{2 * FdsStatFGPeriodSec};  // How often stats are pushed for aggregation into more coarse grained stats.

const std::int32_t FdsStatCollectionWaitMult{2};  // The multiple of FdsStatPushAndAggregatePeriodSec seconds we are willing to wait for remote services to get their stats to us before we go ahead and publish.

/**
 * FdsStatRunForever needs to be set to the value given for
 * FDS_ProtocolInterface::g_common_constants.STAT_STREAM_RUN_FOR_EVER_DURATION.
 * Because of the way Thrift generates this defintiion, it cannot be used directly here
 * for this initialization.
 */
const std::int32_t FdsStatRunForever{-1};  // The duration setting for a stats stream that is to run indefinitely.

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

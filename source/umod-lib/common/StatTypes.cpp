/**
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <StatTypes.h>

namespace fds {

const StatConstants* g_stat_constants = nullptr;

StatConstants::StatConstants() {

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
    FdsStatLTPeriodSec = 24*60*60;  // The period over which "Long-Term" stats are collected - every 24 hours.
    FdsStatLTSlotCnt = 31;  // 30 days + 1

  /**
   * FdsStatFGPeriodSec needs to be set to the value given for
   * FDS_ProtocolInterface::g_common_constants.STAT_STREAM_FINE_GRAINED_FREQUENCY_SECONDS.
   * Because of the way Thrift generates this defintion, it cannot be used directly here
   * for this initialization.
   */
    FdsStatFGPeriodSec = FDS_ProtocolInterface::g_common_constants.STAT_STREAM_FINE_GRAINED_FREQUENCY_SECONDS;  // The period over which "Fine-Grained" stats are collected.
    FdsStatFGPeriodMultToCG = 5;  // The multiple of FG periods that define a CG period.
    FdsStatFGSlotCnt = FdsStatFGPeriodMultToCG + 2;  // Number of "slots" or generations of FG stats we keep in history. We'll keep a few more than necessary to roll into a CG slot.

    FdsStatCGPeriodSec = FdsStatFGPeriodMultToCG * FdsStatFGPeriodSec;  // The period over which "Coarse-Grained" stats are collected
    FdsStatCGPeriodMultToLT = FdsStatLTPeriodSec / FdsStatCGPeriodSec;  // The multiple of CG periods that define a LT period.
    FdsStatCGSlotCnt = FdsStatCGPeriodMultToLT + 2;  // Number of "slots" or generations of CG stats we keep in history. We'll keep a few more than necessary to roll into a LT slot.

    FdsStatPushAndAggregatePeriodSec = 2 * FdsStatFGPeriodSec;  // How often stats are pushed for aggregation into more coarse grained stats.

    FdsStatCollectionWaitMult = 2;  // The multiple of FdsStatPushAndAggregatePeriodSec seconds we are willing to wait for remote services to get their stats to us before we go ahead and publish.

  /**
   * FdsStatRunForever needs to be set to the value given for
   * FDS_ProtocolInterface::g_common_constants.STAT_STREAM_RUN_FOR_EVER_DURATION.
   * Because of the way Thrift generates this defintiion, it cannot be used directly here
   * for this initialization.
   */
    FdsStatRunForever = FDS_ProtocolInterface::g_common_constants.STAT_STREAM_RUN_FOR_EVER_DURATION;  // The duration setting for a stats stream that is to run indefinitely.
}

const StatConstants* StatConstants::singleton() {
    if (g_stat_constants == nullptr) {
        g_stat_constants = new StatConstants();
    }

    return g_stat_constants;
}

} // namespace fds


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
     * to keep in each case as the number necessary to roll up into the
     * next more "coarse-grained" collection plus the number we are willing to
     * skip while we wait and see if we get more stats for those generations from
     * remote services plus a couple more.
     */
    FdsStatLTPeriodSec = 24*60*60;  // The period over which "Long-Term" stats are collected - every 24 hours.
    FdsStatLTSlotCnt = 32;  // 30 days + 2

    FdsStatFGPeriodSec = 60;  // The period over which "Fine-Grained" stats are collected, in seconds.

    FdsStatPushAndAggregatePeriodSec = 2 * FdsStatFGPeriodSec;  // How often stats are pushed for aggregation into more coarse grained stats.
    FdsStatCollectionWaitMult = 2;  // The multiple of FdsStatPushAndAggregatePeriodSec seconds we are willing to wait for remote services to get their stats to us before we go ahead and publish.

    FdsStatFGPeriodMultToCG = 5;  // The multiple of FG periods that define a CG period.
    FdsStatFGSlotCnt = FdsStatFGPeriodMultToCG + (FdsStatCollectionWaitMult*(FdsStatPushAndAggregatePeriodSec/FdsStatFGPeriodSec)) + 2;  // Number of "slots" or generations of FG stats we keep in history.

    FdsStatCGPeriodSec = FdsStatFGPeriodMultToCG * FdsStatFGPeriodSec;  // The period over which "Coarse-Grained" stats are collected
    FdsStatCGPeriodMultToLT = FdsStatLTPeriodSec / FdsStatCGPeriodSec;  // The multiple of CG periods that define a LT period.
    FdsStatCGSlotCnt = FdsStatCGPeriodMultToLT + 2;  // Number of "slots" or generations of CG stats we keep in history.

    /**
     * The smallest period, the expiration of which results in fine-grained stats being streamed. Any larger period
     * must be a multiple of this. And itself, it should be a multiple of FdsStatPushAndAggregatePeriodSec.
     */
    FdsStatFGStreamPeriodFactorSec = FDS_ProtocolInterface::g_common_constants.STAT_STREAM_FINE_GRAINED_FREQUENCY_SECONDS;
    fds_assert(FdsStatFGStreamPeriodFactorSec % FdsStatFGPeriodSec == 0);
    fds_assert(FdsStatFGStreamPeriodFactorSec % FdsStatPushAndAggregatePeriodSec == 0);

    FdsStatRunForever = FDS_ProtocolInterface::g_common_constants.STAT_STREAM_RUN_FOR_EVER_DURATION;  // The duration setting for a stats stream that is to run indefinitely.
}

const StatConstants* StatConstants::singleton() {
    if (g_stat_constants == nullptr) {
        g_stat_constants = new StatConstants();
    }

    return g_stat_constants;
}

} // namespace fds


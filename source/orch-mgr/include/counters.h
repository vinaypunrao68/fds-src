/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_COUNTERS_H_
#define SOURCE_ORCH_MGR_INCLUDE_COUNTERS_H_

#include <fds_counters.h>
namespace fds {
namespace om {
struct Counters : FdsCounters {
    explicit Counters(FdsCountersMgr *mgr);
    ~Counters() = default;

    SimpleNumericCounter volumesBeingCreated;
    SimpleNumericCounter volumesBeingDeleted;
    SimpleNumericCounter numSnapshotsInDeleteQueue;
    SimpleNumericCounter numSnapshotPoliciesScheduled;
};
}  // namespace om
}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_COUNTERS_H_

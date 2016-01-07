/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <counters.h>
namespace fds {
namespace om {
Counters::Counters(FdsCountersMgr *mgr)
            : FdsCounters("om", mgr),
              volumesBeingCreated("volumes.creating.count", this),
              volumesBeingDeleted("volumes.deleting.count", this),
              numSnapshotsInDeleteQueue("snapshots.deleting.count", this),
              numSnapshotPoliciesScheduled("snapshots.policies.count",this) {
}

}  // namespace om
}  // namespace fds

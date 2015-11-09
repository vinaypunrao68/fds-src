/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_COUNTERS_H_
#define SOURCE_DATA_MGR_INCLUDE_COUNTERS_H_

#include <fds_counters.h>
namespace fds {
namespace dm {
struct Counters : FdsCounters {
    Counters(const std::string &id, FdsCountersMgr *mgr) 
            : FdsCounters(id, mgr),
              refscanRunning("dm.refscan.running", this),
              refscanNumVolumes("dm.refscan.num_volumes", this),
              refscanNumTokenFiles("dm.refscan.num_token_files", this),
              refscanLastRun("dm.refscan.lastrun.timestamp", this) {
    }
    ~Counters() = default;

    SimpleNumericCounter refscanRunning;
    SimpleNumericCounter refscanNumVolumes;
    SimpleNumericCounter refscanNumTokenFiles;
    SimpleNumericCounter refscanLastRun;
};
}  // namespace dm
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_COUNTERS_H_

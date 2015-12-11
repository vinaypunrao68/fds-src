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
              refscanNumTokenFiles("dm.refscan.num_tokens", this),
              refscanNumObjects("dm.refscan.num_objects", this),
              refscanLastRun("dm.refscan.lastrun.timestamp", this),
              totalVolumesReceivedMigration("dm.migration.total.volumesReceived", this),
              totalVolumesSentMigration("dm.migration.total.volumesSent", this),
              totalMigrationsAborted("dm.migration.total.aborted", this),
              numberOfActiveMigrExecutors("dm.migrations.active.executors", this),
              numberOfActiveMigrClients("dm.migrations.active.clients", this),
              numberOfOutstandingIOs("dm.migrations.active.outstandingios", this),
              totalSizeOfDataMigrated("dm.migrations.total.BytesMigrated", this),
              timeSpentForCurrentMigration("dm.migrations.current.timeSpent", this),
              timeSpentForAllMigrations("dm.migrations.total.timeSpent", this) {
    }
    ~Counters() = default;

    SimpleNumericCounter refscanRunning;
    SimpleNumericCounter refscanNumVolumes;
    SimpleNumericCounter refscanNumTokenFiles;
    SimpleNumericCounter refscanLastRun;
    SimpleNumericCounter refscanNumObjects;
    SimpleNumericCounter totalVolumesReceivedMigration;
    SimpleNumericCounter totalVolumesSentMigration;
    SimpleNumericCounter totalMigrationsAborted;
    SimpleNumericCounter numberOfActiveMigrExecutors;
    SimpleNumericCounter numberOfActiveMigrClients;
    SimpleNumericCounter numberOfOutstandingIOs;
    SimpleNumericCounter totalSizeOfDataMigrated;
    SimpleNumericCounter timeSpentForCurrentMigration;
    SimpleNumericCounter timeSpentForAllMigrations;
};
}  // namespace dm
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_COUNTERS_H_

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_COUNTERS_H_
#define SOURCE_DATA_MGR_INCLUDE_COUNTERS_H_

#include <fds_counters.h>
namespace fds {
namespace dm {
struct Counters : FdsCounters {
    Counters(const std::string &id, FdsCountersMgr *mgr);
    ~Counters() = default;

    SimpleNumericCounter refscanRunning;
    SimpleNumericCounter refscanNumVolumes;
    SimpleNumericCounter refscanNumTokenFiles;
    SimpleNumericCounter refscanLastRun;
    SimpleNumericCounter refscanNumObjects;

    // Starting with VG mode, because migrations can happen at any time,
    // the total values are not reset, but active values are dynamically changed
    SimpleNumericCounter migrationLastRun;
    SimpleNumericCounter totalVolumesReceivedMigration;
    SimpleNumericCounter totalVolumesSentMigration;
    SimpleNumericCounter totalMigrationsAborted;
    SimpleNumericCounter numberOfActiveMigrExecutors;
    SimpleNumericCounter numberOfActiveMigrClients;
    SimpleNumericCounter numberOfOutstandingIOs;
    SimpleNumericCounter totalSizeOfDataMigrated;
    SimpleNumericCounter timeSpentForCurrentMigration;
    SimpleNumericCounter timeSpentForAllMigrations;
    SimpleNumericCounter totalVolumesToBeMigrated;
    SimpleNumericCounter migrationDMTVersion;

    void clearMigrationCounters();

};
}  // namespace dm
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_COUNTERS_H_

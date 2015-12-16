/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <counters.h>
namespace fds {
namespace dm {
Counters::Counters(const std::string &id, FdsCountersMgr *mgr)
            : FdsCounters(id, mgr),
              refscanRunning("dm.refscan.running", this),
              refscanNumVolumes("dm.refscan.num_volumes", this),
              refscanNumTokenFiles("dm.refscan.num_tokens", this),
              refscanNumObjects("dm.refscan.num_objects", this),
              refscanLastRun("dm.refscan.lastrun.timestamp", this),

              migrationLastRun("dm.migration.lastrun.timestamp", this),
              migrationDMTVersion("dm.migration.dmt.version", this),
              totalVolumesToBeMigrated("dm.migration.volumes.total", this),
              totalVolumesReceivedMigration("dm.migration.volumes.rcvd", this),
              totalVolumesSentMigration("dm.migration.volumes.sent", this),
              totalMigrationsAborted("dm.migration.aborted", this),
              numberOfActiveMigrExecutors("dm.migration.active.executors", this),
              numberOfActiveMigrClients("dm.migration.active.clients", this),
              numberOfOutstandingIOs("dm.migration.active.outstandingios", this),
              totalSizeOfDataMigrated("dm.migration.bytes", this),
              timeSpentForCurrentMigration("dm.migration.current.duration", this),
              timeSpentForAllMigrations("dm.migration.total.duration", this) {
}

void Counters::clearMigrationCounters() {
    migrationLastRun.set(0);
    totalVolumesToBeMigrated.set(0);
    migrationDMTVersion.set(0);
    totalVolumesReceivedMigration.set(0);
    totalVolumesSentMigration.set(0);
    totalMigrationsAborted.set(0);
    numberOfActiveMigrExecutors.set(0);
    numberOfActiveMigrClients.set(0);
    numberOfOutstandingIOs.set(0);
    totalSizeOfDataMigrated.set(0);
    timeSpentForCurrentMigration.set(0);
    timeSpentForAllMigrations.set(0);
}

}  // namespace dm
}  // namespace fds

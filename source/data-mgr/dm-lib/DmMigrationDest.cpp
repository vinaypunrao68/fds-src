/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmMigrationDest.h>
#include <DataMgr.h>
#include <util/stringutils.h>

namespace fds {

DmMigrationDest::DmMigrationDest(int64_t _migrId,
                                 DataMgr &_dm,
                                 NodeUuid &_srcDmUuid,
                                 fpi::FDSP_VolumeDescType& _volDesc,
                                 uint32_t _timeout,
                                 DmMigrationExecutorDoneCb cleanUp)
: DmMigrationExecutor(_dm,
                      _srcDmUuid,
                      _volDesc,
                      _migrId,
                      false,
                      cleanUp,
                      _timeout)
{
    logStr = util::strformat("[DmMigrationDest volId: %ld]", volumeUuid.get());
}

Error
DmMigrationDest::start()
{
    Error err(ERR_OK);
    LOGMIGRATE << "migrationid: " << migrationId
            << " starting Destination blob diff for volume: " << volumeUuid;

    {
        fds_scoped_lock lock(progressLock);
        migrationProgress = STATICMIGRATION_IN_PROGRESS;
    }

    // true - volumeGroupMode
    processInitialBlobFilterSet();

    return err;
}

void
DmMigrationDest::testStaticMigrationComplete()
{
    {
        fds_scoped_lock lock(progressLock);
        fds_verify(migrationProgress == STATICMIGRATION_IN_PROGRESS &&
                   deltaBlobDescsSeqNum.isSeqNumComplete());

        migrationProgress = MIGRATION_COMPLETE;
    }

#if 0
    if (migrDoneCb) {
        dataMgr.getModuleProvider()->proc_thrpool()->schedule(migrDoneCb,
                                                              srcDmSvcUuid,
                                                              volDesc.volUUID,
                                                              ERR_OK);
    }
#endif
}

void DmMigrationDest::routeAbortMigration()
{
    abortMigration();
}

void
DmMigrationDest::abortMigration()
{
    LOGWARN << logString() << "Abort migration called.";
    {
        fds_scoped_lock lock(progressLock);
        migrationProgress = MIGRATION_ABORTED;
    }
    // TODO(Rao): Handle this properly
    if (migrDoneCb) {
        dataMgr.getModuleProvider()->proc_thrpool()->schedule(migrDoneCb,
                                                              srcDmSvcUuid,
                                                              volDesc.volUUID,
                                                              ERR_DM_MIGRATION_ABORTED);
    }
}
} // namespace fds

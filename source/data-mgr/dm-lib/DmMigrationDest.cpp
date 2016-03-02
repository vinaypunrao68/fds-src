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
                      _timeout),
                      idleTimeoutTaskPtr(nullptr)
{
    logStr = util::strformat("[DmMigrationDest volId: %ld]", volumeUuid.get());
    version = VolumeGroupConstants::VERSION_INVALID;
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

    auto timer = dataMgr.getModuleProvider()->getTimer();
    auto task = [this] () {
        /* Immediately post to threadpool so we don't hold up timer thread */
        MODULEPROVIDER()->proc_thrpool()->schedule(
                &DmMigrationDest::migrationIdleTimeoutCheck, this);
    };
    idleTimeoutTaskPtr = SHPTR<FdsTimerTask>(new FdsTimerFunctionTask(task));
    timer->scheduleRepeated(idleTimeoutTaskPtr,
            std::chrono::seconds(dataMgr.dmMigrationMgr->getidleTimeoutSecs()));

    return err;
}

void
DmMigrationDest::testStaticMigrationComplete()
{
    {
        fds_scoped_lock lock(progressLock);
        fds_verify(migrationProgress == STATICMIGRATION_IN_PROGRESS);
        if (deltaBlobDescsSeqNum.isSeqNumComplete()) {
            migrationProgress = MIGRATION_COMPLETE;
            cancelIdleTimer();
        }
    }
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
        cancelIdleTimer();
    }
    // TODO(Rao): Handle this properly
    if (migrDoneCb) {
        dataMgr.getModuleProvider()->proc_thrpool()->schedule(migrDoneCb,
                                                              srcDmSvcUuid,
                                                              volDesc.volUUID,
                                                              ERR_DM_MIGRATION_ABORTED);
    }
}
Error
DmMigrationDest::checkVolmetaVersion(const int32_t version)
{
    Error err(ERR_OK);
    auto volMeta = dataMgr.getVolumeMeta(volumeUuid);
    if (volMeta == nullptr) {
        err = ERR_INVALID_VERSION;
    } else if (version == VolumeGroupConstants::VERSION_INVALID) {
        LOGDEBUG << "Received version: " << version;
        fds_assert(!"Source shouldn't send invalid version");
        err = ERR_INVALID;
    } else if (volMeta->getVersion() != version) {
        LOGDEBUG << logString() << " invalid version mismatch: " << version <<
                " expecting: " << volMeta->getVersion();
        err = ERR_INVALID_VERSION;
    }
    return err;
}

void
DmMigrationDest::migrationIdleTimeoutCheck()
{
    if (isMigrationIdle(util::getTimeStampSeconds())) {
        abortMigration();
    }
}

void
DmMigrationDest::cancelIdleTimer() {
    if (idleTimeoutTaskPtr != nullptr) {
        dataMgr.getModuleProvider()->getTimer()->cancel(idleTimeoutTaskPtr);
        idleTimeoutTaskPtr.reset();
    }
}

} // namespace fds

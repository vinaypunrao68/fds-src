/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmIoReq.h>
#include <DmMigrationMgr.h>
#include <DmMigrationExecutor.h>
#include <DmMigrationClient.h>

namespace fds {

DmMigrationMgr::DmMigrationMgr(DataMgr& _dataMgr)
    : dataMgr(_dataMgr),
      OmStartMigrCb(NULL)
{
    // TODO(sean): Need start migration message callback.  Called when the migration completes or aborts.
	migrState = MIGR_IDLE;
	cleanUpInProgress = false;
}

DmMigrationMgr::~DmMigrationMgr()
{

}

Error
DmMigrationMgr::createMigrationExecutor(const NodeUuid& srcDmUuid,
										fpi::FDSP_VolumeDescType &vol,
										MigrationType& migrationType)
{
	Error err(ERR_OK);

    SCOPEDWRITE(migrExecutorLock);
	/**
	 * Make sure that this isn't an ongoing operation.
	 * Otherwise, OM bug?
	 */
	auto search = executorMap.find(fds_volid_t(vol.volUUID));
	if (search != executorMap.end()) {
		LOGMIGRATE << "Migration for volume " << vol.vol_name << " is a duplicated request.";
		err = ERR_DUPLICATE;
	} else {
		/**
		 * Create a new instance of migration Executor
		 */
		LOGMIGRATE << "Creating migration instance for volume id=: " << vol.volUUID
                   << " name=" << vol.vol_name;

		executorMap.emplace(fds_volid_t(vol.volUUID),
				            DmMigrationExecutor::unique_ptr(new DmMigrationExecutor(dataMgr,
														                            srcDmUuid,
                                                                                    vol,
														                            std::bind(&DmMigrationMgr::migrationExecutorDoneCb,
																                              this,
                                                                                              std::placeholders::_1,
																                              std::placeholders::_2))));
	}
	return err;
}

Error
DmMigrationMgr::startMigration(fpi::CtrlNotifyDMStartMigrationMsgPtr &migrationMsg)
{
	Error err(ERR_OK);

	NodeUuid srcDmSvcUuid;

	// TODO(Neil) fix this
	MigrationType localMigrationType(MIGR_DM_ADD_NODE);

	/**
	 * Make sure we're in a state able to dispatch migration.
	 */
	err = activateStateMachine();

	if (err != ERR_OK) {
		return err;
	}

	for (std::vector<fpi::DMVolumeMigrationGroup>::iterator vmg = migrationMsg->migrations.begin();
		 vmg != migrationMsg->migrations.end();
         ++vmg) {

		for (std::vector<fpi::FDSP_VolumeDescType>::iterator vdt = vmg->VolDescriptors.begin();
			 vdt != vmg->VolDescriptors.end();
             ++vdt) {


			srcDmSvcUuid.uuid_set_val(vmg->source.svc_uuid);

            LOGMIGRATE << "Pull Volume ID: " << vdt->volUUID
                       << " Name: " << vdt->vol_name
                       << " from SrcDmSvcUuid: " << vmg->source.svc_uuid;

			err = createMigrationExecutor(srcDmSvcUuid,
                                          *vdt,
									      localMigrationType);
			if (!err.OK()) {
				LOGERROR << "Error creating migrating task.  err=" << err;
				return err;
			}
		}
	}

	/**
	 * For now, execute one at a time. Improve this later.
	 */
	for (DmMigrationExecMap::iterator mit = executorMap.begin();
		 mit != executorMap.end();
         ++mit) {
		mit->second->startMigration();
		if (isMigrationAborted()) {
			/**
			 * Abort everything
			 */
			err = ERR_DM_MIGRATION_ABORTED;
			break;
		}
	}
	return err;
}

Error
DmMigrationMgr::activateStateMachine()
{
	Error err(ERR_OK);

	MigrationState expectedState(MIGR_IDLE);

	if (!std::atomic_compare_exchange_strong(&migrState, &expectedState, MIGR_IN_PROGRESS)) {
		// If not ABORTED state, then something wrong with OM FSM sending this migration msg
		fds_verify(isMigrationAborted());
		if (isMigrationAborted()) {
			/**
			 * If migration is in aborted state, don't do anything. Let the cleanup occur.
			 * Once the cleanup is done, the state should go back to MIGR_IDLE.
			 * In the meantime, do not serve anymore migrations.
			 */
			fds_verify(cleanUpInProgress);
			LOGMIGRATE << "DM MigrationMgr cannot receive new requests as it is "
					<< "currently in an error state and being cleaned up.";
			err = ERR_NOT_READY;
		}
	} else {
		/**
		 * Migration should be idle
		 */
		fds_verify(ongoingMigrationCnt() == 0);
	}
	return err;
}

void
DmMigrationMgr::migrationExecutorDoneCb(fds_volid_t volId, const Error &result)
{
	SCOPEDWRITE(migrExecutorLock);
	MigrationState expectedState(MIGR_IN_PROGRESS);
	/**
	 * Make sure we can find it
	 */
	DmMigrationExecMap::iterator mapIter = executorMap.find(volId);
	fds_verify(mapIter != executorMap.end());

	/**
	 * TODO(Neil): error handling
	 */
	if (!result.OK()) {
		fds_verify(isMigrationInProgress() || isMigrationAborted());
		fds_verify(std::atomic_compare_exchange_strong(&migrState, &expectedState, MIGR_ABORTED));
		LOGERROR << "Volume=" << volId << " failed migration with error: " << result;
	} else {
		/**
		 * Normal exit. Really doesn't do much as we're waiting for the clients to come back.
		 */
	}
}

}  // namespace fds

/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmIoReq.h>
#include <DmMigrationMgr.h>
#include <DmMigrationExecutor.h>
#include <DmMigrationClient.h>

namespace fds {

DmMigrationMgr::DmMigrationMgr(DmIoReqHandler *DmReqHandle)
    : DmReqHandler(DmReqHandle),
      OmStartMigrCb(NULL)
{
	migrState = MIGR_IDLE;
}

DmMigrationMgr::~DmMigrationMgr()
{

}

Error
DmMigrationMgr::startMigration(fpi::CtrlNotifyDMStartMigrationMsgPtr &migrationMsg)
{
	Error err(ERR_OK);
	for (std::vector<fpi::DMVolumeMigrationGroup>::iterator vmg = migrationMsg->migrations.begin();
			vmg != migrationMsg->migrations.end(); vmg++) {
		LOGMIGRATE << "DM MigrationMgr " << MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid().svc_uuid
				<< "received migration group. Pull from node: " <<
				vmg->source.svc_uuid << " the following volumes: ";
		for (std::vector<fpi::FDSP_VolumeDescType>::iterator vdt = vmg->VolDescriptors.begin();
				vdt != vmg->VolDescriptors.end(); vdt++) {
			LOGMIGRATE << "Volume ID: " << vdt->volUUID << " name: " << vdt->vol_name;
		}
	}

	return err;
}
}  // namespace fds

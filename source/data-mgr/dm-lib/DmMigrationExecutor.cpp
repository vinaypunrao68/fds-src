/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmIoReq.h>
#include <DmMigrationExecutor.h>

namespace fds {

DmMigrationExecutor::DmMigrationExecutor(DmIoReqHandler* _DmReqHandle,
    							 	 	 const NodeUuid& _srcDmUuid,
	 	 								 fpi::FDSP_VolumeDescType& _volDesc,
										 DmMigrationExecutorDoneCb _callback)
    : DmReqHandler(_DmReqHandle),
      srcDmSvcUuid(_srcDmUuid),
      volDesc(_volDesc),
      migrDoneCb(_callback)
{
	LOGMIGRATE << "DmMigrationExecutor: " << volDesc;
}

DmMigrationExecutor::~DmMigrationExecutor()
{
}

Error
DmMigrationExecutor::startMigration()
{
	Error err(ERR_OK);

	LOGMIGRATE << "starting migration for VolDesc: " << volDesc;

    /**
     * TODO(Sean):  Fire off message
     */

	if (migrDoneCb) {
		migrDoneCb(volDesc.volUUID, err);
	}

    return err;
}

}  // namespace fds

/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmIoReq.h>
#include <DmMigrationExecutor.h>

namespace fds {

DmMigrationExecutor::DmMigrationExecutor(DmIoReqHandler* DmReqHandle,
    							 	 	 NodeUuid& srcDmUuid,
	 	 								 const NodeUuid& mySvcUuid,
	 	 								 fpi::FDSP_VolumeDescType& vol,
										 DmMigrationExecutorDoneHandler handle)
    : DmReqHandler(DmReqHandle)
{
	LOGDEBUG << "Migration executor received for volume " << vol.vol_name;

    _srcDmUuid = srcDmUuid;
    _mySvcUuid = mySvcUuid;
    _vol = vol;
    migrDoneHandler = handle;
}

DmMigrationExecutor::~DmMigrationExecutor()
{
}

fpi::FDSP_VolumeDescType
DmMigrationExecutor::getVolDesc()
{
	return _vol;
}

void
DmMigrationExecutor::execute()
{
	Error err(ERR_OK);
	/**
	 * For now, shoot blanks
	 */
	LOGDEBUG << "Firing migrate message for vol " << _vol.vol_name;
	if (migrDoneHandler) {
		migrDoneHandler(fds_uint64_t(_vol.volUUID), err);
	}
}
}  // namespace fds

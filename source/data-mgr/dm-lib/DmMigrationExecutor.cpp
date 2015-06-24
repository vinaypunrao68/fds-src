/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmIoReq.h>
#include <DmMigrationExecutor.h>

namespace fds {

DmMigrationExecutor::DmMigrationExecutor(DmIoReqHandler* _DmReqHandle,
    							 	 	 NodeUuid& _srcDmUuid,
	 	 								 const NodeUuid& _mySvcUuid,
	 	 								 fpi::FDSP_VolumeDescType& _vol,
										 DmMigrationExecutorDoneHandler _handle)
    : DmReqHandler(_DmReqHandle)
{
	LOGMIGRATE << "Migration executor received for volume " << _vol.vol_name;

	srcDmUuid = boost::make_shared<NodeUuid> (_srcDmUuid);
	mySvcUuid = boost::make_shared<NodeUuid> (_mySvcUuid);
	vol = boost::make_shared<fpi::FDSP_VolumeDescType> (_vol);
    migrDoneHandler = _handle;
}

DmMigrationExecutor::~DmMigrationExecutor()
{
}

boost::shared_ptr<fpi::FDSP_VolumeDescType>
DmMigrationExecutor::getVolDesc()
{
	return vol;
}

void
DmMigrationExecutor::execute()
{
	Error err(ERR_OK);
	/**
	 * For now, shoot blanks
	 */
	LOGMIGRATE << "Firing migrate message for vol " << vol->vol_name;
	if (migrDoneHandler) {
		migrDoneHandler(fds_uint64_t(vol->volUUID), err);
	}
}
}  // namespace fds

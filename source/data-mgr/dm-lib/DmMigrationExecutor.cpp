/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmIoReq.h>
#include <DmMigrationExecutor.h>

#include "fds_module_provider.h"
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>

namespace fds {

DmMigrationExecutor::DmMigrationExecutor(DmIoReqHandler* _DmReqHandle,
    							 	 	 NodeUuid& _srcDmUuid,
	 	 								 const NodeUuid& _mySvcUuid,
	 	 								 fpi::FDSP_VolumeDescType& _vol,
										 DmMigrationExecutorDoneHandler _handle)
    : DmReqHandler(_DmReqHandle), migrDoneHandler(_handle)
{
	LOGMIGRATE << "Migration executor received for volume " << _vol.vol_name;

	srcDmUuid = boost::make_shared<NodeUuid> (_srcDmUuid);
	mySvcUuid = boost::make_shared<NodeUuid> (_mySvcUuid);
	vol = boost::make_shared<fpi::FDSP_VolumeDescType> (_vol);
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
	 * For now, shoot fake bullets for testing.
	 * TODO(Neil) remove this when real messages can be sent.
	 */
	LOGMIGRATE << "Firing migrate message for vol " << vol->vol_name;

    fpi::ResyncInitialBlobFilterSetMsgPtr message (new fpi::ResyncInitialBlobFilterSetMsg());
    message->volume_id = vol->volUUID;
    /**
     * DUMMY - replace with real message getter later
     */
    fpi::BlobFilterSetEntry oneEntry;
    for (int i=0; i < 5; i++) {
    	oneEntry.blob_id = i+10;
    	oneEntry.sequence_id = i;
    }
    message->blob_filter_set.push_back(oneEntry);
    // End dummy

    auto svcMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    fpi::SvcUuid tempUuid;
    tempUuid.svc_uuid = srcDmUuid->uuid_get_val();
    auto request = svcMgr->newEPSvcRequest(tempUuid);
    request->setPayload (FDSP_MSG_TYPEID (fpi::ResyncInitialBlobFilterSetMsg), message);
    request->invoke();


	if (migrDoneHandler) {
		migrDoneHandler(fds_uint64_t(vol->volUUID), err);
	}
}
}  // namespace fds

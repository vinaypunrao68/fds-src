/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmMigrationExecutor.h>
#include <fdsp/dm_types_types.h>
#include <fdsp/dm_api_types.h>

#include "fds_module_provider.h"
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>

namespace fds {

DmMigrationExecutor::DmMigrationExecutor(DataMgr& _dataMgr,
    							 	 	 const NodeUuid& _srcDmUuid,
	 	 								 fpi::FDSP_VolumeDescType& _volDesc,
										 const fds_bool_t& _autoIncrement,
										 DmMigrationExecutorDoneCb _callback)
	: dataMgr(_dataMgr),
      srcDmSvcUuid(_srcDmUuid),
      volDesc(_volDesc),
	  autoIncrement(_autoIncrement),
      migrDoneCb(_callback)
{
    volumeUuid = volDesc.volUUID;
	LOGMIGRATE << "Migration executor received for volume ID " << volDesc;
}

DmMigrationExecutor::~DmMigrationExecutor()
{
}

Error
DmMigrationExecutor::startMigration()
{
	Error err(ERR_OK);

	LOGMIGRATE << "starting migration for VolDesc: " << volDesc;

    /** TODO(Sean):
     * process_vol_add() doesn't quiet work for all cases for the use of DM migration.
     * it doesn't work for resync on restart.
     *
     * we should be:
     * 1) check if the volume catalog exists.  if so, add to the volume map in inactive mode.
     * or
     * 2) if the volume catalog doesn't exists, create and add to the volume map in inactive mode.
     *
     * unfortunately, process_vol_add() doesn't fit this model.  So, we need to add a new function
     * that does this.  for now, just use this, since we are dealing with static migration with
     * add node only.
     */
#if 0
	// TODO - this currently doesn't work right and fails the StaticMigration test
    err = dataMgr._process_add_vol(dataMgr.getPrefix() + std::to_string(volumeUuid.get()),
                                   volumeUuid,
                                   &volDesc,
                                   false);
#endif
    if (!err.ok()) {
        LOGERROR << "process_add_vol failed on volume=" << volumeUuid
                 << " with error=" << err;
        return err;
    }

    /**
     * If the volume is successfully created with the given volume descriptor, process and generate the
     * initial blob filter set to be sent to the source DM.
     */
    err = processInitialBlobFilterSet();
    if (!err.ok()) {
        LOGERROR << "processInitialBlobFilterSet failed on volume=" << volumeUuid
                 << " with error=" << err;
        return err;
    }

	if (migrDoneCb) {
		migrDoneCb(volDesc.volUUID, err);
	}
    return err;
}

Error
DmMigrationExecutor::processInitialBlobFilterSet()
{
	Error err(ERR_OK);
	LOGMIGRATE << "starting migration for VolDesc: " << volDesc;

    /**
     * create and initialize message for initial filter set.
     */
    fpi::ResyncInitialBlobFilterSetMsgPtr filterSet(new fpi::ResyncInitialBlobFilterSetMsg());
    filterSet->volumeId = volumeUuid.get();

    LOGMIGRATE << "processing to get list of <blobid, seqnum> for volume=" << volumeUuid;
    /**
     * Get the list of <blobid, seqnum> for a volume associted with this executor.
     */
#if 0
	// TODO - this currently doesn't work right and fails the StaticMigration test
    err = dataMgr.timeVolCat_->queryIface()->getAllBlobsWithSequenceId(fds_volid_t(volumeUuid),
                                                                       filterSet->blobFilterMap);
#endif
    if (!err.ok()) {
        LOGERROR << "failed to generatate list of <blobid, seqnum> for volume=" << volumeUuid
                 <<" with error=" << err;
        return err;
    }

    /**
     * If successfully generated the initial filter set, send it to the source DM.
     */
    auto asyncInitialBlobSetReq = gSvcRequestPool->newEPSvcRequest(srcDmSvcUuid.toSvcUuid());
    asyncInitialBlobSetReq->setPayload(FDSP_MSG_TYPEID(fpi::ResyncInitialBlobFilterSetMsg), filterSet);
    asyncInitialBlobSetReq->setTimeoutMs(0);
    asyncInitialBlobSetReq->invoke();

    return err;
}

}  // namespace fds
